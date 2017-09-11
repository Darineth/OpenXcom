#ifndef OPENXCOM_SOLDIERAVATAR_H
#define OPENXCOM_SOLDIERAVATAR_H

#include "Soldier.h"

namespace OpenXcom
{

/**
 * Represents a soldier's avatar/paperdoll.
 * Avatar is a combination of soldier's gender, look and look variant
 */
class SoldierAvatar
{
private:
	std::string _avatarName;
	SoldierGender _gender;
	SoldierLook _look;
	int _lookVariant;
public:
	/// Creates a new empty avatar.
	SoldierAvatar();
	/// Creates a new avatar.
	SoldierAvatar(const std::string &avatarName, SoldierGender gender, SoldierLook look, int lookVariant);
	/// Cleans up the avatar.
	~SoldierAvatar();
	/// returns avatar name
	std::string getAvatarName() const;
	/// returns gender
	SoldierGender getGender() const;
	/// returns look
	SoldierLook getLook() const;
	/// returns look variant
	int getLookVariant() const;
};

}

#endif
