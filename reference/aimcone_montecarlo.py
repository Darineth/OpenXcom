"""
Monte-Carlo calibration for the DX aim-cone trajectory model
(plans/Feature-AimConeTrajectory.md). Run with plain `python`; no dependencies.

Kept in reference/ for future balance passes: when tuning the cone constants
(TUNING, the acc^2/50 shaping, the soldier-cone x2 multiplier) or a weapon's
baseAccuracy, edit the constants/sweeps below, re-run, and compare tables.
If the engine-side code changes, keep the native port here in sync with
Projectile::applyAccuracy and the cone port in sync with the implementation.

Simulates both firing models against a standing-soldier target and prints
hit-probability vs. distance tables:

  1. NATIVE scatter model - faithful port of Projectile::applyAccuracy
     (Projectile.cpp:494-557, classic non-uniform spread, extendLine=true,
     no range dropoff, i.e. default fields + battleUFOExtenderAccuracy off).
  2. AIM-CONE model - per the design doc (Feature-AimConeTrajectory.md),
     adopted constants (decision 11, "V3", Jul 2026):
       soldier cone: sigma_s = 0.437 / (soldierAcc^2 / 50) * 1.4826 * 2
       weapon  cone: sigma_w = 0.437 / (weaponAcc^2 / 75) * 1.4826
     soldierAcc floored at 20, weaponAcc floored at 1, each sampled
     deflection clamped at 3 sigma, random azimuth per deflection.
     The weapon cone is quadratic in baseAccuracy, NORMALIZED AT 75: it
     matches the legacy fork's linear cone exactly at baseAccuracy 75 and
     spreads harder away from it (stronger modding knob, same global
     balance). Set WEAPON_SHAPE = "linear" below to compare against the
     legacy linear scaling. Tuning concepts: the soldier x2 multiplier is
     the GLOBAL LETHALITY dial; the weapon exponent/normalization is the
     KNOB STRENGTH dial - they are orthogonal.

Geometry: shooter fires along +X. Target = axis-aligned box silhouette
(cylinder approximated as |y| <= R, |z| <= H/2) centred on the aim point at
D voxels (16 voxels per tile). Hit = deflected ray crosses the target plane
inside the silhouette. Small-angle plane test; fine for the angles involved.

soldierAcc (percent) = Firing stat x shotMode/100  (kneel/wound/1-hand
modifiers just scale the same number; not swept here).
"""

import math
import random

random.seed(20260702)

VOXELS_PER_TILE = 16
TARGET_RADIUS = 4.5       # voxels; typical personal-armor LOFT cylinder
TARGET_HALF_HEIGHT = 11.0 # voxels; ~22-voxel standing soldier
TRIALS = 40000

MAD_TO_SIGMA = 1.4826
TUNING = 0.437
SOLDIER_ACC_FLOOR = 20
WEAPON_ACC_FLOOR = 1
CLAMP_SIGMAS = 3.0
SOLDIER_CONE_MULT = 2.0     # global lethality dial (legacy x2)
WEAPON_QUAD_NORM = 75.0     # knob strength dial: sigma_w normalization point
WEAPON_SHAPE = "quad75"     # adopted; "linear" = legacy fork scaling


# ---------------------------------------------------------------- native ---

def native_scatter_hit(accuracy_pct, dist_tiles):
    """One shot under Projectile::applyAccuracy (classic spread). Returns hit?"""
    D = dist_tiles * VOXELS_PER_TILE
    accuracy = accuracy_pct / 100.0  # call sites divide by accuracyDivider

    # fire along +X: xDist=D, yDist=0, zDist=0
    x_dist, y_dist, z_dist = D, 0, 0
    if x_dist // 2 <= y_dist:
        xy_shift = x_dist // 4 + y_dist
    else:
        xy_shift = (x_dist + y_dist) // 2
    if xy_shift <= z_dist:
        z_shift = xy_shift // 2 + z_dist
    else:
        z_shift = xy_shift + z_dist // 2

    # int deviation = RNG(0,100) - accuracy*100  (C++ truncation toward zero)
    deviation = int(random.randint(0, 100) - accuracy * 100.0)
    if deviation >= 0:
        deviation += 50   # miss cloud
    else:
        deviation += 10
    deviation = max(1, (z_shift * deviation) // 200)

    # displaced target point (target starts at (D, 0, 0))
    px = D + random.randint(0, deviation) - deviation // 2
    py = 0 + random.randint(0, deviation) - deviation // 2
    pz = 0 + random.randint(0, deviation // 2) // 2 - deviation // 8

    # extendLine: ray = origin -> displaced point; cross plane x = D
    if px <= 0:
        return False  # deflected behind/at shooter: cannot reach target plane
    t = D / px
    y_at, z_at = py * t, pz * t
    return abs(y_at) <= TARGET_RADIUS and abs(z_at) <= TARGET_HALF_HEIGHT


# -------------------------------------------------------------- aim-cone ---

def _sample_deflection(sigma):
    """Gaussian polar deflection, clamped at 3 sigma (sign folds into azimuth)."""
    theta = random.gauss(0.0, sigma)
    return max(-CLAMP_SIGMAS * sigma, min(CLAMP_SIGMAS * sigma, theta))


def _rotate_random_azimuth(v, theta):
    """Deflect unit vector v by polar angle theta around a uniform-random azimuth."""
    # build orthonormal basis (v, e1, e2)
    if abs(v[0]) < 0.9:
        ref = (1.0, 0.0, 0.0)
    else:
        ref = (0.0, 1.0, 0.0)
    e1 = (v[1] * ref[2] - v[2] * ref[1],
          v[2] * ref[0] - v[0] * ref[2],
          v[0] * ref[1] - v[1] * ref[0])
    n = math.sqrt(e1[0] ** 2 + e1[1] ** 2 + e1[2] ** 2)
    e1 = (e1[0] / n, e1[1] / n, e1[2] / n)
    e2 = (v[1] * e1[2] - v[2] * e1[1],
          v[2] * e1[0] - v[0] * e1[2],
          v[0] * e1[1] - v[1] * e1[0])
    phi = random.uniform(0.0, 2.0 * math.pi)
    ct, st = math.cos(theta), math.sin(theta)
    cp, sp = math.cos(phi), math.sin(phi)
    return (v[0] * ct + (e1[0] * cp + e2[0] * sp) * st,
            v[1] * ct + (e1[1] * cp + e2[1] * sp) * st,
            v[2] * ct + (e1[2] * cp + e2[2] * sp) * st)


def cone_sigmas(soldier_acc_pct, weapon_acc):
    s = max(SOLDIER_ACC_FLOOR, soldier_acc_pct)
    w = max(WEAPON_ACC_FLOOR, weapon_acc)
    sigma_s = TUNING / (s * s / 50.0) * MAD_TO_SIGMA * SOLDIER_CONE_MULT
    if WEAPON_SHAPE == "quad75":
        sigma_w = TUNING / (w * w / WEAPON_QUAD_NORM) * MAD_TO_SIGMA
    else:  # "linear" - legacy fork scaling
        sigma_w = TUNING / w * MAD_TO_SIGMA
    return sigma_s, sigma_w


def aim_cone_hit(soldier_acc_pct, weapon_acc, dist_tiles):
    """One shot under the aim-cone model. Returns hit?"""
    D = dist_tiles * VOXELS_PER_TILE
    sigma_s, sigma_w = cone_sigmas(soldier_acc_pct, weapon_acc)

    v = (1.0, 0.0, 0.0)
    v = _rotate_random_azimuth(v, _sample_deflection(sigma_s))  # soldier cone
    v = _rotate_random_azimuth(v, _sample_deflection(sigma_w))  # weapon cone

    if v[0] <= 0.0:
        return False
    t = D / v[0]
    y_at, z_at = v[1] * t, v[2] * t
    return abs(y_at) <= TARGET_RADIUS and abs(z_at) <= TARGET_HALF_HEIGHT


# ------------------------------------------------------------- reporting ---

def hit_pct(shot_fn, trials=TRIALS):
    hits = sum(1 for _ in range(trials) if shot_fn())
    return 100.0 * hits / trials


def effective_range_50(soldier_acc_pct, weapon_acc, lo=1.0, hi=200.0):
    """Distance (tiles) where aim-cone hit chance crosses 50% (bisection)."""
    def p(d):
        return hit_pct(lambda: aim_cone_hit(soldier_acc_pct, weapon_acc, d),
                       trials=8000)
    if p(lo) < 50.0:
        return 0.0
    if p(hi) >= 50.0:
        return float('inf')
    for _ in range(18):
        mid = (lo + hi) / 2.0
        if p(mid) >= 50.0:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2.0


DISTANCES = [2, 5, 8, 10, 15, 20, 25, 30, 40]

SHOOTERS = [("Rookie", 40), ("Average", 60), ("Veteran", 80), ("Elite", 110)]
MODES = [("Auto 35%", 35), ("Snap 60%", 60), ("Aimed 110%", 110)]
BASE_ACC_DEFAULT = 75      # legacy RuleItem::_baseAccuracy default
BASE_ACC_SWEEP = [40, 60, 75, 100, 150, 300]


def fmt_row(label, vals):
    return label.ljust(26) + "".join(f"{v:7.1f}" for v in vals)


def header(title):
    print()
    print("=" * 100)
    print(title)
    print("=" * 100)
    print("dist (tiles)".ljust(26) + "".join(f"{d:7d}" for d in DISTANCES))
    print("-" * 100)


def main():
    print(f"Target silhouette: cylinder R={TARGET_RADIUS} voxels, "
          f"height {2 * TARGET_HALF_HEIGHT:.0f} voxels; {TRIALS} shots/cell")
    print(f"Cone constants: {TUNING} / (sAcc^2/50) * {MAD_TO_SIGMA} * "
          f"{SOLDIER_CONE_MULT}  |  weapon shape '{WEAPON_SHAPE}' "
          f"(norm {WEAPON_QUAD_NORM:.0f});  floor sAcc>={SOLDIER_ACC_FLOOR}, "
          f"clamp {CLAMP_SIGMAS:.0f} sigma")

    # --- Table 1: native vs cone, per shooter x mode, weapon baseAccuracy 75
    for mode_name, mode_pct in MODES:
        header(f"{mode_name}  |  soldierAcc = Firing x {mode_pct}/100  |  "
               f"cone weapon baseAccuracy = {BASE_ACC_DEFAULT}")
        for shooter_name, firing in SHOOTERS:
            folded = firing * mode_pct / 100.0
            native = [hit_pct(lambda d=d: native_scatter_hit(folded, d))
                      for d in DISTANCES]
            cone = [hit_pct(lambda d=d: aim_cone_hit(folded, BASE_ACC_DEFAULT, d))
                    for d in DISTANCES]
            sig_s, sig_w = cone_sigmas(folded, BASE_ACC_DEFAULT)
            print(fmt_row(f"{shooter_name} F{firing} native "
                          f"(acc {folded:.0f}%)", native))
            print(fmt_row(f"{shooter_name} F{firing} cone   "
                          f"(ss{math.degrees(sig_s):4.1f} sw{math.degrees(sig_w):3.1f} deg)",
                          cone))
            print()

    # --- Table 2: baseAccuracy sweep (weapon cone) at Average F60, Snap 60
    folded = 60 * 60 / 100.0
    header(f"Weapon baseAccuracy sweep  |  Average shooter F60, Snap 60% "
           f"(soldierAcc {folded:.0f})")
    for base_acc in BASE_ACC_SWEEP:
        cone = [hit_pct(lambda d=d: aim_cone_hit(folded, base_acc, d))
                for d in DISTANCES]
        _, sig_w = cone_sigmas(folded, base_acc)
        print(fmt_row(f"baseAccuracy {base_acc:3d} "
                      f"(sw {math.degrees(sig_w):4.2f} deg)", cone))
    native = [hit_pct(lambda d=d: native_scatter_hit(folded, d)) for d in DISTANCES]
    print(fmt_row("native scatter (ref)", native))

    # --- Table 3: 50%-hit effective range per combo (the readout definition)
    print()
    print("=" * 100)
    print(f"Aim-cone effective range (50% hit), tiles  |  "
          f"weapon baseAccuracy = {BASE_ACC_DEFAULT}")
    print("=" * 100)
    print("shooter".ljust(16) + "".join(f"{m:>14s}" for m, _ in MODES))
    for shooter_name, firing in SHOOTERS:
        vals = []
        for _, mode_pct in MODES:
            folded = firing * mode_pct / 100.0
            vals.append(effective_range_50(folded, BASE_ACC_DEFAULT))
        print(shooter_name.ljust(16) +
              "".join(f"{v:14.1f}" for v in vals))


if __name__ == "__main__":
    main()
