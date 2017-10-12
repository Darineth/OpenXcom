/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <iomanip>
#include <tuple>
#include <algorithm>

#include "Logger.h"
#include "Options.h"
#include "Script.h"
#include "ScriptBind.h"
#include "Surface.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"
#include "Exception.h"


namespace OpenXcom
{

////////////////////////////////////////////////////////////
//						arg definition
////////////////////////////////////////////////////////////
#define MACRO_QUOTE(...) __VA_ARGS__

#define MACRO_COPY_4(Func, Pos) \
	Func((Pos) + 0x0) \
	Func((Pos) + 0x1) \
	Func((Pos) + 0x2) \
	Func((Pos) + 0x3)
#define MACRO_COPY_16(Func, Pos) \
	MACRO_COPY_4(Func, (Pos) + 0x00) \
	MACRO_COPY_4(Func, (Pos) + 0x04) \
	MACRO_COPY_4(Func, (Pos) + 0x08) \
	MACRO_COPY_4(Func, (Pos) + 0x0C)
#define MACRO_COPY_64(Func, Pos) \
	MACRO_COPY_16(Func, (Pos) + 0x00) \
	MACRO_COPY_16(Func, (Pos) + 0x10) \
	MACRO_COPY_16(Func, (Pos) + 0x20) \
	MACRO_COPY_16(Func, (Pos) + 0x30)
#define MACRO_COPY_256(Func, Pos) \
	MACRO_COPY_64(Func, (Pos) + 0x00) \
	MACRO_COPY_64(Func, (Pos) + 0x40) \
	MACRO_COPY_64(Func, (Pos) + 0x80) \
	MACRO_COPY_64(Func, (Pos) + 0xC0)


////////////////////////////////////////////////////////////
//						proc definition
////////////////////////////////////////////////////////////
[[gnu::always_inline]]
static inline void addShade_h(int& reg, const int& var)
{
	const int newShade = (reg & 0xF) + var;
	if (newShade > 0xF)
	{
		// so dark it would flip over to another color - make it black instead
		reg = 0xF;
		return;
	}
	else if (newShade > 0)
	{
		reg = (reg & 0xF0) | newShade;
		return;
	}
	reg &= 0xF0;
	//prevent overflow to 0 or another color - make it white instead
	if (!reg || newShade < 0)
		reg = 0x01;
}

[[gnu::always_inline]]
static inline RetEnum mulAddMod_h(int& reg, const int& mul, const int& add, const int& mod)
{
	const int a = reg * mul + add;
	if (mod)
	{
		reg = (a % mod + mod) % mod;
		return RetContinue;
	}
	return RetError;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_rect_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += period;
	if (reg > size)
		reg = 0;
	else
		reg = max;
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_saw_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += period;
	if (reg > size)
		reg = 0;
	else if (reg > max)
		reg = max;
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_tri_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += period;
	if (reg > size)
		reg = 0;
	else
	{
		if (reg > size/2)
			reg = size - reg;
		if (reg > max)
			reg = max;
	}
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum call_func_h(ScriptWorkerBase& c, FuncCommon func, const Uint8* d, ProgPos& p)
{
	auto t = p;
	auto r = func(c, d, t);
	p = t;
	return r;
}


/**
 * Main macro defining all available operation in script engine.
 * @param IMPL macro function that access data. Take 3 args: Name, definition of operation and delcaration of it's arguments.
 */
#define MACRO_PROC_DEFINITION(IMPL) \
	/*	Name,		Implementation,													End excecution,				Args,					Description */ \
	IMPL(exit,		MACRO_QUOTE({													return RetEnd;		}),		(ScriptWorkerBase&),	"") \
	\
	IMPL(goto,		MACRO_QUOTE({ Prog = Label1;									return RetContinue; }),		(ScriptWorkerBase& c, ProgPos& Prog, ProgPos Label1),	"") \
	\
	IMPL(set,		MACRO_QUOTE({ Reg0 = Data1;										return RetContinue; }),		(ScriptWorkerBase& c, int& Reg0, int Data1),	"arg1 = arg2") \
	\
	IMPL(clear,		MACRO_QUOTE({ Reg0 = 0;											return RetContinue; }),		(ScriptWorkerBase& c, int& Reg0),				"arg1 = 0") \
	\
	IMPL(test_le,	MACRO_QUOTE({ Prog = (A <= B) ? LabelTrue : LabelFalse;			return RetContinue; }),		(ProgPos& Prog, int A, int B, ProgPos LabelTrue, ProgPos LabelFalse),	"") \
	IMPL(test_eq,	MACRO_QUOTE({ Prog = (A == B) ? LabelTrue : LabelFalse;			return RetContinue; }),		(ProgPos& Prog, int A, int B, ProgPos LabelTrue, ProgPos LabelFalse),	"") \
	\
	IMPL(swap,		MACRO_QUOTE({ std::swap(Reg0, Reg1);							return RetContinue; }),		(int& Reg0, int& Reg1),				"Swap value of arg1 and arg2") \
	IMPL(add,		MACRO_QUOTE({ Reg0 += Data1;									return RetContinue; }),		(int& Reg0, int Data1),				"arg1 = arg1 + arg2") \
	IMPL(sub,		MACRO_QUOTE({ Reg0 -= Data1;									return RetContinue; }),		(int& Reg0, int Data1),				"arg1 = arg1 - arg2") \
	IMPL(mul,		MACRO_QUOTE({ Reg0 *= Data1;									return RetContinue; }),		(int& Reg0, int Data1),				"arg1 = arg1 * arg2") \
	IMPL(muldiv,	MACRO_QUOTE({ Reg0 = (Reg0 * Data1) / Data2;					return RetContinue; }),		(int& Reg0, int Data1, int Data2),	"arg1 = (arg1 * arg2) / arg3") \
	\
	IMPL(aggregate,	MACRO_QUOTE({ Reg0 = Reg0 + Data1 * Data2;						return RetContinue; }),		(int& Reg0, int Data1, int Data2),			"arg1 = arg1 + (arg2 * arg3)") \
	IMPL(offset,	MACRO_QUOTE({ Reg0 = Reg0 * Data1 + Data2;						return RetContinue; }),		(int& Reg0, int Data1, int Data2),			"arg1 = (arg1 * arg2) + arg3") \
	IMPL(offsetmod,	MACRO_QUOTE({ return mulAddMod_h(Reg0, Mul1, Add2, Mod3);							}),		(int& Reg0, int Mul1, int Add2, int Mod3),	"arg1 = ((arg1 * arg2) + arg3) % arg4") \
	\
	IMPL(div,		MACRO_QUOTE({ if (!Data1) return RetError; Reg0 /= Data1;		return RetContinue; }),		(int& Reg0, int Data1),		"arg1 = arg1 / arg2") \
	IMPL(mod,		MACRO_QUOTE({ if (!Data1) return RetError; Reg0 %= Data1;		return RetContinue; }),		(int& Reg0, int Data1),		"arg1 = arg1 % arg2") \
	\
	IMPL(shl,		MACRO_QUOTE({ Reg0 <<= Data1;									return RetContinue; }),		(int& Reg0, int Data1),		"Left bit shift of arg1 by arg2") \
	IMPL(shr,		MACRO_QUOTE({ Reg0 >>= Data1;									return RetContinue; }),		(int& Reg0, int Data1),		"Rigth bit shift of arg1 by arg2") \
	\
	IMPL(abs,			MACRO_QUOTE({ Reg0 = std::abs(Reg0);							return RetContinue; }),		(int& Reg0),						"Absolute value of arg1") \
	IMPL(limit,			MACRO_QUOTE({ Reg0 = std::max(std::min(Reg0, Data2), Data1);	return RetContinue; }),		(int& Reg0, int Data1, int Data2),	"Correct value in arg1 that is always betwean arg2 and arg3") \
	IMPL(limit_upper,	MACRO_QUOTE({ Reg0 = std::min(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1),				"Correct value in arg1 that is always lesser than arg2") \
	IMPL(limit_lower,	MACRO_QUOTE({ Reg0 = std::max(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1),				"Correct value in arg1 that is always greater than arg2") \
	\
	IMPL(wavegen_rect,	MACRO_QUOTE({ return wavegen_rect_h(Reg0, Period1, Size2, Max3);				}),		(int& Reg0, int Period1, int Size2, int Max3),		"Square wave function, arg1 - argument and result, arg2 - pieriod, arg3 - legth of square, arg4 - heigth of square") \
	IMPL(wavegen_saw,	MACRO_QUOTE({ return wavegen_saw_h(Reg0, Period1, Size2, Max3);					}),		(int& Reg0, int Period1, int Size2, int Max3),		"Saw wave function, arg1 - argument and result, arg2 - pieriod, arg3 - size of saw, arg4 - cap value") \
	IMPL(wavegen_tri,	MACRO_QUOTE({ return wavegen_tri_h(Reg0, Period1, Size2, Max3);					}),		(int& Reg0, int Period1, int Size2, int Max3),		"Triangle wave function, arg1 - argument and result, arg2 - pieriod, arg3 - size of triangle, arg4 - cap value") \
	\
	IMPL(get_color,		MACRO_QUOTE({ Reg0 = Data1 >> 4;							return RetContinue; }),		(int& Reg0, int Data1),		"Get color part to arg1 of pixel color in arg2") \
	IMPL(set_color,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF) | (Data1 << 4);			return RetContinue; }),		(int& Reg0, int Data1),		"Set color part to pixel color in arg1") \
	IMPL(get_shade,		MACRO_QUOTE({ Reg0 = Data1 & 0xF;							return RetContinue; }),		(int& Reg0, int Data1),		"Get shade part to arg1 of pixel color in arg2") \
	IMPL(set_shade,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF0) | (Data1 & 0xF);			return RetContinue; }),		(int& Reg0, int Data1),		"Set color part to pixel color in arg1") \
	IMPL(add_shade,		MACRO_QUOTE({ addShade_h(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1),		"Add value of shade to pixel color in arg1") \
	\
	IMPL(call,			MACRO_QUOTE({ return call_func_h(c, func, d, p);								}),		(FuncCommon func, const Uint8* d, ScriptWorkerBase& c, ProgPos& p),		"") \


////////////////////////////////////////////////////////////
//					function definition
////////////////////////////////////////////////////////////

namespace
{

/**
 * Macro returning name of function
 */
#define MACRO_FUNC_ID(id) Func_##id

/**
 * Macro used for creating functions from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_FUNC(NAME, Impl, Args, ...) \
	struct MACRO_FUNC_ID(NAME) \
	{ \
		[[gnu::always_inline]] \
		static RetEnum func Args \
			Impl \
	};

MACRO_PROC_DEFINITION(MACRO_CREATE_FUNC)

#undef MACRO_CREATE_FUNC


struct Func_test_eq_null
{
	[[gnu::always_inline]]
	static RetEnum func (ProgPos& Prog, std::nullptr_t, std::nullptr_t, ProgPos LabelTrue, ProgPos)
	{
		Prog = LabelTrue;
		return RetContinue;
	}
};

struct Func_debug_impl_int
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, int i)
	{
		c.log_buffer_add(std::to_string(i));
		return RetContinue;
	}
};

struct Func_debug_flush
{
	[[gnu::always_inline]]
	static RetEnum func (ScriptWorkerBase& c, ProgPos& p)
	{
		c.log_buffer_flush(p);
		return RetContinue;
	}
};

} //namespace

////////////////////////////////////////////////////////////
//					Proc_Enum definition
////////////////////////////////////////////////////////////

/**
 * Macro returning enum form ProcEnum
 */
#define MACRO_PROC_ID(id) Proc_##id

/**
 * Macro used for creating ProcEnum from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_PROC_ENUM(NAME, ...) \
	MACRO_PROC_ID(NAME), \
	Proc_##NAME##_end = MACRO_PROC_ID(NAME) + helper::FuncGroup<MACRO_FUNC_ID(NAME)>::ver() - 1,

/**
 * Enum storing id of all avaliable operations in script engine
 */
enum ProcEnum : Uint8
{
	MACRO_PROC_DEFINITION(MACRO_CREATE_PROC_ENUM)
	Proc_EnumMax,
};

#undef MACRO_CREATE_PROC_ENUM

////////////////////////////////////////////////////////////
//					core loop function
////////////////////////////////////////////////////////////

/**
 * Core function in script engine used to executing scripts
 * @param proc array storing operation of script
 * @return Result of executing script
 */
static inline void scriptExe(ScriptWorkerBase& data, const Uint8* proc)
{
	ProgPos curr = ProgPos::Start;
	//--------------------------------------------------
	//			helper macros for this function
	//--------------------------------------------------
	#define MACRO_FUNC_ARRAY(NAME, ...) + helper::FuncGroup<MACRO_FUNC_ID(NAME)>::FuncList{}
	#define MACRO_FUNC_ARRAY_LOOP(POS) \
		case (POS): \
		{ \
			using currType = helper::GetType<func, POS>; \
			const auto p = proc + (int)curr; \
			curr += currType::offset; \
			const auto ret = currType::func(data, p, curr); \
			if (ret != RetContinue) \
			{ \
				if (ret == RetEnd) \
					goto endLabel; \
				else \
				{ \
					curr += - currType::offset - 1; \
					goto errorLabel; \
				} \
			} \
			else \
				continue; \
		}
	//--------------------------------------------------

	using func = decltype(MACRO_PROC_DEFINITION(MACRO_FUNC_ARRAY));

	while (true)
	{
		switch (proc[(int)curr++])
		{
		MACRO_COPY_256(MACRO_FUNC_ARRAY_LOOP, 0)
		}
	}

	//--------------------------------------------------
	//			removing helper macros
	//--------------------------------------------------
	#undef MACRO_FUNC_ARRAY_LOOP
	#undef MACRO_FUNC_ARRAY
	//--------------------------------------------------

	errorLabel:
	static int bugCount = 0;
	if (++bugCount < 100)
	{
		Log(LOG_ERROR) << "Invaild script operation for OpId: " << std::hex << std::showbase << (int)proc[(int)curr] <<" at "<< (int)curr;
	}

	endLabel:
	return;
}


////////////////////////////////////////////////////////////
//						Script class
////////////////////////////////////////////////////////////

/**
 * Bliting one surface to another using script.
 * @param src source surface.
 * @param dest destination surface.
 * @param x x offset of source surface.
 * @param y y offset of source surface.
 */
void ScriptWorkerBlit::executeBlit(Surface* src, Surface* dest, int x, int y, int shade, bool half)
{
	ShaderMove<Uint8> srcShader(src, x, y);
	if (half)
	{
		GraphSubset g = srcShader.getDomain();
		g.beg_x = g.end_x/2;
		srcShader.setDomain(g);
	}
	if (_proc)
	{
		if (_events)
		{
			ShaderDrawFunc(
				[&](Uint8& dest, const Uint8& src)
				{
					if (src)
					{
						ScriptWorkerBlit::Output arg = { src, dest };
						set(arg);
						auto ptr = _events;
						while (*ptr)
						{
							reset(arg);
							scriptExe(*this, ptr->data());
							++ptr;
						}
						++ptr;

						reset(arg);
						scriptExe(*this, _proc);

						while (*ptr)
						{
							reset(arg);
							scriptExe(*this, ptr->data());
							++ptr;
						}
						++ptr;

						get(arg);
						if (arg.getFirst()) dest = arg.getFirst();
					}
				},
				ShaderSurface(dest, 0, 0),
				srcShader
			);
		}
		else
		{
			ShaderDrawFunc(
				[&](Uint8& dest, const Uint8& src)
				{
					if (src)
					{
						ScriptWorkerBlit::Output arg = { src, dest };
						set(arg);
						scriptExe(*this, _proc);
						get(arg);
						if (arg.getFirst()) dest = arg.getFirst();
					}
				},
				ShaderSurface(dest, 0, 0),
				srcShader
			);
		}
	}
	else
	{
		ShaderDraw<helper::StandardShade>(ShaderSurface(dest, 0, 0), srcShader, ShaderScalar(shade));
	}
}

/**
 * Execute script with two arguments.
 * @return Result value from script.
 */
void ScriptWorkerBase::executeBase(const Uint8* proc)
{
	if (proc)
	{
		scriptExe(*this, proc);
	}
}

/**
 * Add text to log buffer.
 */
void ScriptWorkerBase::log_buffer_add(const std::string& s)
{
	if (!log_buffer.empty())
	{
		log_buffer += " ";
	}
	log_buffer += s;
}

/**
 * Flush buffer to log file.
 */
void ScriptWorkerBase::log_buffer_flush(ProgPos& p)
{
	constexpr int limit = 500;
	static int limit_count = 0;
	if (++limit_count < limit)
	{
		Logger log;
		log.get(LOG_DEBUG) << "Script debug log at " << std::hex << std::showbase << std::setw(8) << ((int)p) << ": " << log_buffer;
		log_buffer.clear();
	}
	else if (limit_count == limit)
	{
		Logger log;
		log.get(LOG_DEBUG) << "Script debug log limit reach";
	}
}

////////////////////////////////////////////////////////////
//					Helper functions
////////////////////////////////////////////////////////////

namespace
{

/**
 * Test for validaty of arguments.
 */
bool validOverloadProc(const ScriptRange<ScriptRange<ArgEnum>>& overload)
{
	for (auto& p : overload)
	{
		for (auto& pp : p)
		{
			if (pp == ArgInvalid)
			{
				return false;
			}
		}
	}
	return true;
}

/**
 * Display arguments
 */
template<typename T, typename F>
std::string displayArgs(const ScriptParserBase* spb, const ScriptRange<T>& range, F getType)
{
	std::string result = "";
	for (auto& p : range)
	{
		if (p)
		{
			auto type = getType(p);
			result += "[";
			result += spb->getTypePrefix(type);
			result += spb->getTypeName(type).toString();
			result += "] ";
		}
	}
	if (!result.empty())
	{
		result.pop_back();
	}
	return result;
}

/**
 * Display arguments
 */
std::string displayOverloadProc(const ScriptParserBase* spb, const ScriptRange<ScriptRange<ArgEnum>>& overload)
{
	return displayArgs(spb, overload, [](const ScriptRange<ArgEnum>& o) { return *o.begin(); });
}

/**
 * Accept all arguments.
 */
int overloadBuildinProc(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end)
{
	return 1;
}

/**
 * Reject all arguments.
 */
int overloadInvalidProc(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end)
{
	return 0;
}

/**
 * Verify arguments.
 */
int overloadCustomProc(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto tempSorce = 255;
	auto curr = begin;
	for (auto& currOver : spd.overloadArg)
	{
		if (curr == end)
		{
			return 0;
		}
		const auto size = currOver.size();
		if (size)
		{
			if (*curr)
			{
				int oneArgTempScore = 0;
				for (auto& o : currOver)
				{
					oneArgTempScore = std::max(oneArgTempScore, ArgCompatible(o, curr->type, size - 1));
				}
				tempSorce = std::min(tempSorce, oneArgTempScore);
			}
			++curr;
		}
	}
	if (curr != end)
	{
		return 0;
	}
	return tempSorce;
}
/**
 * Helper choosing corrct overload function to call.
 */
bool callOverloadProc(ParserWriter& ph, const ScriptRange<ScriptProcData>& proc, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (!proc)
	{
		return false;
	}
	if (std::distance(begin, end) > ScriptMaxArg)
	{
		return false;
	}

	int bestSorce = 0;
	const ScriptProcData* bestValue = nullptr;
	for (auto& p : proc)
	{
		int tempSorce = p.overload(p, begin, end);
		if (tempSorce)
		{
			if (tempSorce == bestSorce)
			{
				bestValue = nullptr;
			}
			else if (tempSorce > bestSorce)
			{
				bestSorce = tempSorce;
				bestValue = &p;
			}
		}
	}
	if (bestSorce)
	{
		if (bestValue)
		{
			if ((*bestValue)(ph, begin, end) == false)
			{
				Log(LOG_ERROR) << "Error in maching arguments for operator '" + proc.begin()->name.toString() + "'";
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			Log(LOG_ERROR) << "Conflicting overloads for operator '" + proc.begin()->name.toString() + "' for:";
			Log(LOG_ERROR) << "  " << displayArgs(&ph.parser, ScriptRange<ScriptRefData>{ begin, end }, [](const ScriptRefData& r){ return r.type; });
			Log(LOG_ERROR) << "Excepted:";
			for (auto& p : proc)
			{
				if (p.parserArg != nullptr && p.overloadArg)
				{
					Log(LOG_ERROR) << "  " << displayOverloadProc(&ph.parser, p.overloadArg);
				}
			}
			return false;
		}
	}
	else
	{
		Log(LOG_ERROR) << "Can't match overload for operator '" + proc.begin()->name.toString() + "' for:";
		Log(LOG_ERROR) << "  " << displayArgs(&ph.parser, ScriptRange<ScriptRefData>{ begin, end }, [](const ScriptRefData& r){ return r.type; });
		Log(LOG_ERROR) << "Excepted:";
		for (auto& p : proc)
		{
			if (p.parserArg != nullptr && p.overloadArg)
			{
				Log(LOG_ERROR) << "  " << displayOverloadProc(&ph.parser, p.overloadArg);
			}
		}
		return false;
	}
}

/**
 * Helper used to parse line for build in function.
 */
template<Uint8 procId, typename FuncGroup>
bool parseBuildinProc(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	auto opPos = ph.pushProc(procId);
	int ver = FuncGroup::parse(ph, begin, end);
	if (ver >= 0)
	{
		ph.updateProc(opPos, ver);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Helper used to parse line for custom functions.
 */
bool parseCustomProc(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	using argFunc = typename helper::ArgSelector<FuncCommon>::type;
	using argRaw = typename helper::ArgSelector<const Uint8*>::type;
	static_assert(helper::FuncGroup<Func_call>::ver() == argRaw::ver(), "Invalid size");
	static_assert(std::is_same<helper::GetType<helper::FuncGroup<Func_call>, 0>, argFunc>::value, "Invalid first argument");
	static_assert(std::is_same<helper::GetType<helper::FuncGroup<Func_call>, 1>, argRaw>::value, "Invalid second argument");

	auto opPos = ph.pushProc(Proc_call);

	auto funcPos = ph.pushReserved<FuncCommon>();
	auto argPosBegin = ph.getCurrPos();

	auto argType = spd.parserArg(ph, begin, end);

	if (argType < 0)
	{
		return false;
	}

	auto argPosEnd = ph.getCurrPos();
	ph.updateReserved<FuncCommon>(funcPos, spd.parserGet(argType));

	size_t diff = ph.getDiffPos(argPosBegin, argPosEnd);
	for (int i = 0; i < argRaw::ver(); ++i)
	{
		size_t off = argRaw::offset(i);
		if (off >= diff)
		{
			//aligin proc to fit fixed size.
			ph.push(off-diff);
			ph.updateProc(opPos, i);
			return true;
		}
	}
	return false;
}

constexpr size_t ConditionSize = 6;
const ScriptRef ConditionNames[ConditionSize] =
{
	ScriptRef{ "eq" }, ScriptRef{ "neq" },
	ScriptRef{ "le" }, ScriptRef{ "gt" },
	ScriptRef{ "ge" }, ScriptRef{ "lt" },
};

constexpr size_t ConditionSpecialSize = 2;
const ScriptRef ConditionSpecNames[ConditionSpecialSize] =
{
	ScriptRef{ "or" },
	ScriptRef{ "and" },
};


/**
 * Helper used of condintion opearations.
 */
bool parseConditionImpl(ParserWriter& ph, ScriptRefData truePos, ScriptRefData falsePos, const ScriptRefData* begin, const ScriptRefData* end)
{
	constexpr size_t TempArgsSize = 4;

	if (std::distance(begin, end) != 3)
	{
		Log(LOG_ERROR) << "Invaild length of condition arguments";
		return false;
	}

	ScriptRefData conditionArgs[TempArgsSize] =
	{
		begin[1],
		begin[2],
		truePos, //success
		falsePos, //failure
	};

	bool equalFunc = false;
	size_t i = 0;
	for (; i < ConditionSize; ++i)
	{
		if (begin[0].name == ConditionNames[i])
		{
			if (i < 2) equalFunc = true;
			if (i & 1) std::swap(conditionArgs[2], conditionArgs[3]); //negate condition result
			if (i >= 4) std::swap(conditionArgs[0], conditionArgs[1]); //swap condition args
			break;
		}
	}
	if (i == ConditionSize)
	{
		Log(LOG_ERROR) << "Unknown condition: '" + begin[0].name.toString() + "'";
		return false;
	}

	const auto proc = ph.parser.getProc(ScriptRef{ equalFunc ? "test_eq" : "test_le" });
	if (callOverloadProc(ph, proc, std::begin(conditionArgs), std::end(conditionArgs)) == false)
	{
		Log(LOG_ERROR) << "Unsupported operator: '" + begin[0].name.toString() + "'";
		return false;
	}

	return true;
}

/**
 * Parse `or` or `and` conditions.
 */
bool parseFullConditionImpl(ParserWriter& ph, ScriptRefData falsePos, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (std::distance(begin, end) <= 1)
	{
		Log(LOG_ERROR) << "Invaild length of condition arguments";
		return false;
	}
	const auto truePos = ph.addLabel();
	const auto orFunc = begin[0].name == ConditionSpecNames[0];
	const auto andFunc = begin[0].name == ConditionSpecNames[1];
	if (orFunc || andFunc)
	{
		++begin;
		for (; std::distance(begin, end) > 3; begin += 3)
		{
			auto temp = ph.addLabel();
			if (orFunc)
			{
				if (!parseConditionImpl(ph, truePos, temp, begin, begin + 3))
				{
					return false;
				}
			}
			else
			{
				if (!parseConditionImpl(ph, temp, falsePos, begin, begin + 3))
				{
					return false;
				}
			}
			ph.setLabel(temp, ph.getCurrPos());
		}
	}
	if (!parseConditionImpl(ph, truePos, falsePos, begin, end))
	{
		return false;
	}

	ph.setLabel(truePos, ph.getCurrPos());
	return true;
}

/**
 * Parser of `if` operation.
 */
bool parseIf(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	ParserWriter::Block block = { BlockIf, ph.addLabel(), ph.addLabel() };
	ph.codeBlocks.push_back(block);

	return parseFullConditionImpl(ph, block.nextLabel, begin, end);
}

/**
 * Parser of `else` operation.
 */
bool parseElse(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (ph.codeBlocks.empty() || ph.codeBlocks.back().type != BlockIf)
	{
		Log(LOG_ERROR) << "Unexpected 'else'";
		return false;
	}

	ParserWriter::Block& block = ph.codeBlocks.back();

	ph.pushProc(Proc_goto);
	ph.pushLabelTry(block.finalLabel);

	ph.setLabel(block.nextLabel, ph.getCurrPos());
	if (std::distance(begin, end) == 0)
	{
		block.nextLabel = block.finalLabel;
		block.type = BlockElse;
		return true;
	}
	else
	{
		block.nextLabel = ph.addLabel();
		return parseFullConditionImpl(ph, block.nextLabel, begin, end);
	}
}

/**
 * Parser of `end` operation.
 */
bool parseEnd(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (ph.codeBlocks.empty())
	{
		Log(LOG_ERROR) << "Unexpected 'end'";
		return false;
	}
	if (std::distance(begin, end) != 0)
	{
		Log(LOG_ERROR) << "Unexpected symbols after 'end'";
		return false;
	}

	ParserWriter::Block block = ph.codeBlocks.back();
	ph.codeBlocks.pop_back();

	if (block.nextLabel.getValue<int>() != block.finalLabel.getValue<int>())
	{
		ph.setLabel(block.nextLabel, ph.getCurrPos());
	}
	ph.setLabel(block.finalLabel, ph.getCurrPos());
	return true;
}

/**
 * Parser of `var` operation that define local variables.
 */
bool parseVar(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (ph.codeBlocks.size() > 0)
	{
		Log(LOG_ERROR) << "Can't define variables in code blocks";
		return false;
	}

	auto spec = ArgSpecVar;
	if (begin != end)
	{
		if (begin[0].name == ScriptRef{ "ptr" })
		{
			spec = spec | ArgSpecPtr;
			++begin;
		}
		else if (begin[0].name == ScriptRef{ "ptre" })
		{
			spec = spec | ArgSpecPtrE;
			++begin;
		}
	}
	auto size = std::distance(begin, end);
	if (size < 2 || 3 < size)
	{
		Log(LOG_ERROR) << "Invaild length of 'var' definition";
		return false;
	}

	// adding new custom variables of type selected type.
	auto type_curr = ph.parser.getType(begin[0].name);
	if (type_curr)
	{
		if (type_curr->size == 0 && !(spec & ArgSpecPtr))
		{
			Log(LOG_ERROR) << "Can't create variable of type '" << begin->name.toString() << "'";
			return false;
		}

		++begin;
		if (begin[0].type != ArgInvalid || begin[0].name.find('.') != std::string::npos || ph.addReg(begin[0].name, ArgSpecAdd(type_curr->type, spec)) == false)
		{
			Log(LOG_ERROR) << "Invalid variable name '" << begin[0].name.toString() << "'";
			return false;
		}
		if (size == 3)
		{
			ScriptRefData setArgs[] =
			{
				ph.getReferece(begin[0].name),
				begin[1],
			};
			const auto proc = ph.parser.getProc(ScriptRef{ "set" });
			return callOverloadProc(ph, proc, std::begin(setArgs), std::end(setArgs));
		}
		else
		{
			ScriptRefData setArgs[] =
			{
				ph.getReferece(begin[0].name),
			};
			const auto proc = ph.parser.getProc(ScriptRef{ "clear" });
			return callOverloadProc(ph, proc, std::begin(setArgs), std::end(setArgs));
		}
		return true;
	}
	else
	{
		Log(LOG_ERROR) << "Invalid type '" << begin[0].name.toString() << "'";
		return false;
	}
}

/**
 * Parse return statment.
 */
bool parseReturn(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	const auto size = std::distance(begin, end);
	const auto returnSize = ph.parser.haveEmptyReturn() ? 0 : ph.parser.getParamSize();
	if (returnSize != size)
	{
		Log(LOG_ERROR) << "Invaild length of returns arguments";
		return false;
	}

	ScriptRefData outputRegsData[ScriptMaxOut] = { };
	RegEnum currValueIndex[ScriptMaxOut] = { };
	RegEnum newValueIndex[ScriptMaxOut] = { };

	for (int i = 0; i < size; ++i)
	{
		outputRegsData[i] = *ph.parser.getParamData(i);
		if (begin[i].isValueType<RegEnum>() && !ArgCompatible(outputRegsData[i].type, begin[i].type, 1))
		{
			Log(LOG_ERROR) << "Invaild return argument '" + begin[i].name.toString() + "'";
			return false;
		}
		currValueIndex[i] = outputRegsData[i].getValue<RegEnum>();
		newValueIndex[i] = begin[i].getValueOrDefulat<RegEnum>(RegInvaild);
		if (currValueIndex[i] == newValueIndex[i])
		{
			currValueIndex[i] = RegInvaild;
		}
	}

	// matching return arguments to return register,
	// sometimes curret value in one register is needed in another.
	// we need find order of assigments that will not lose any value.
	auto any_changed = true;
	auto all_free = false;
	while (!all_free && any_changed)
	{
		all_free = true;
		any_changed = false;
		for (int i = 0; i < size; ++i)
		{
			if (currValueIndex[i] == RegInvaild)
			{
				continue;
			}
			auto free = true;
			for (int j = 0; j < size; ++j)
			{
				if (i != j && currValueIndex[i] == newValueIndex[j])
				{
					free = false;
					break;
				}
			}
			if (free)
			{
				any_changed = true;
				currValueIndex[i] = RegInvaild;
				ScriptRefData temp[] = { outputRegsData[i], begin[i] };

				const auto proc = ph.parser.getProc(ScriptRef{ "set" });
				if (!callOverloadProc(ph, proc, std::begin(temp), std::end(temp)))
				{
					Log(LOG_ERROR) << "Invaild return argument '" + begin[i].name.toString() + "'";
					return false;
				}
			}
			else
			{
				all_free = false;
			}
		}
	}

	if (!all_free)
	{
		// now we can have only cases where return register have circular dependencies:
		// e.g. A need B, B need C and C need A.
		// using swap we can fix circular dependencies.
		for (int i = 0; i < size; ++i)
		{
			if (currValueIndex[i] == RegInvaild)
			{
				continue;
			}

			for (int j = 0; j < size; ++j)
			{
				if (i != j && newValueIndex[i] == currValueIndex[j])
				{
					ScriptRefData temp[] = { outputRegsData[i], outputRegsData[j] };

					const auto proc = ph.parser.getProc(ScriptRef{ "swap" });
					if (!callOverloadProc(ph, proc, std::begin(temp), std::end(temp)))
					{
						return false;
					}

					// now value from 'i' is in 'j'
					currValueIndex[j] = currValueIndex[i];
					currValueIndex[i] = RegInvaild;
					break;
				}
			}
		}
	}
	ph.pushProc(Proc_exit);
	return true;
}

/**
 * Parse `debug_log` operator.
 */
bool parseDebugLog(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	if (!Options::debug)
	{
		return true;
	}

	for (auto i = begin; i != end; ++i)
	{
		const auto proc = ph.parser.getProc(ScriptRef{ "debug_impl" });
		if (!callOverloadProc(ph, proc, i, std::next(i)))
		{
			Log(LOG_ERROR) << "Invaild debug argument '" + i->name.toString() + "'";
			return false;
		}
	}

	const auto proc = ph.parser.getProc(ScriptRef{ "debug_flush" });
	return proc.size() == 1 && (*proc.begin())(ph, nullptr, nullptr);
}

/**
 * Dummy operation.
 */
bool parseDummy(const ScriptProcData& spd, ParserWriter& ph, const ScriptRefData* begin, const ScriptRefData* end)
{
	Log(LOG_ERROR) << "Reserved operation for future use";
	return false;
}

/**
 * Add new value to sorted vector by name.
 * @param vec Vector with data.
 * @param name Name of new data.
 * @param value Data to add.
 */
template<typename R>
void addSortHelper(std::vector<R>& vec, R value)
{
	vec.push_back(value);
	std::sort(vec.begin(), vec.end(), [](const R& a, const R& b) { return ScriptRef::compare(a.name, b.name) < 0; });
}

/**
 * Get bound of value, upper od lower based on template parameter.
 * @param begin begin of sorted range.
 * @param end end of sorted range.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Finded iterator or end iterator.
 */
template<bool upper, typename R>
R* boundSortHelper(R* begin, R* end, ScriptRef prefix, ScriptRef postfix = {})
{
	constexpr int limit = upper ? 1 : 0;
	if (postfix)
	{
		const auto size = prefix.size();
		const auto total_size = size + postfix.size();
		return std::partition_point(begin, end,
			[&](const R& a)
			{
				const auto curr = a.name.size();
				if (curr < total_size)
				{
					return true;
				}
				else if (curr == total_size)
				{
					const auto comp  = ScriptRef::compare(a.name.substr(0, size), prefix);
					return comp < 0 || (comp == 0 && ScriptRef::compare(a.name.substr(size), postfix) < limit);
				}
				else
				{
					return false;
				}
			}
		);
	}
	else
	{
		return std::partition_point(begin, end, [&](const R& a){ return ScriptRef::compare(a.name, prefix) < limit; });
	}
}

/**
 * Helper function finding data by name (that can be merge from two parts).
 * @param begin begin of sorted range.
 * @param end end of sorted range.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Finded data or null.
 */
template<typename R>
R* findSortHelper(R* begin, R* end, ScriptRef prefix, ScriptRef postfix = {})
{
	auto f = boundSortHelper<false>(begin, end, prefix, postfix);
	if (f != end)
	{
		if (postfix)
		{
			const auto size = prefix.size();
			if (f->name.substr(0, size) == prefix && f->name.substr(size) == postfix)
			{
				return &*f;
			}
		}
		else
		{
			if (f->name == prefix)
			{
				return &*f;
			}
		}
	}
	return nullptr;
}

/**
 * Helper function finding data by name (that can be merge from two parts).
 * @param vec Vector with values.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Finded data or null.
 */
template<typename R>
const R* findSortHelper(const std::vector<R>& vec, ScriptRef prefix, ScriptRef postfix = {})
{
	return findSortHelper(vec.data(), vec.data() + vec.size(), prefix, postfix);
}

/**
 * Helper function finding data by name (that can be merge from two parts).
 * @param vec Vector with values.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Finded data or null.
 */
template<typename R>
R* findSortHelper(std::vector<R>& vec, ScriptRef prefix, ScriptRef postfix = {})
{
	return findSortHelper(vec.data(), vec.data() + vec.size(), prefix, postfix);
}

/**
 * Calculate space used by reg of that type.
 * @param parser
 * @param type
 * @return Size in bytes.
 */
size_t getRegSize(const ScriptParserBase& parser, ArgEnum type)
{
	auto t = parser.getType(type);
	if (t == nullptr)
	{
		return 0;
	}
	else
	{
		return ArgIsPtr(type) ? sizeof(void*) : t->size;
	}
}

/**
 * Add new string to colection and return reference to it.
 * @param list List where strings are stored.
 * @param s New string to add.
 * @return Reference to strored string.
 */
ScriptRef addString(std::vector<std::vector<char>>& list, const std::string& s)
{
	std::vector<char> refData;
	refData.assign(s.begin(), s.end());
	ScriptRef ref{ refData.data(), refData.data() + refData.size() };

	//we need use char vector becasue its guarante that pointer in ref will not get invalidated when names list grown.
	list.push_back(std::move(refData));
	return ref;
}

} //namespace

////////////////////////////////////////////////////////////
//				ParserWriter helpers
////////////////////////////////////////////////////////////

/**
 * Token type
 */
enum TokenEnum
{
	TokenNone,
	TokenInvaild,
	TokenColon,
	TokenSemicolon,
	TokenSymbol,
	TokenNumber,
};

/**
 * Struct represents position of token in input string
 */
class SelectedToken : public ScriptRef
{
	/// type of this token.
	TokenEnum _type;

public:

	/// Default constructor.
	SelectedToken() : ScriptRef{ }, _type{ TokenNone }
	{

	}

	/// Constructor from range.
	SelectedToken(TokenEnum type, ScriptRef range) : ScriptRef{ range }, _type{ type }
	{

	}

	/// Get token type.
	TokenEnum getType() const
	{
		return _type;
	}

	/// Convert token to script ref.
	ScriptRefData parse(const ParserWriter& ph) const
	{
		if (getType() == TokenNumber)
		{
			auto str = toString();
			int value = 0;
			size_t offset = 0;
			std::stringstream ss(str);
			if (str[0] == '-' || str[0] == '+')
				offset = 1;
			if (str.size() > 2 + offset && str[offset] == '0' && (str[offset + 1] == 'x' || str[offset + 1] == 'X'))
				ss >> std::hex;
			if ((ss >> value))
				return ScriptRefData{ *this, ArgInt, value };
		}
		else if (getType() == TokenSymbol)
		{
			auto ref = ph.getReferece(*this);
			if (ref)
				return ref;
		}
		return ScriptRefData{ *this, ArgInvalid };
	}

};

class ScriptRefTokens : public ScriptRef
{
public:
	/// Using default constructors.
	using ScriptRef::ScriptRef;

	/// Extract new token from current object.
	SelectedToken getNextToken(TokenEnum excepted = TokenNone);
};


/**
 * Function extracting token form range
 * @param excepted what token type we expecting now
 * @return extracted token
 */
SelectedToken ScriptRefTokens::getNextToken(TokenEnum excepted)
{
	//groups of different types of ASCII characters
	const Uint8 none = 1;
	const Uint8 spec = 2;
	const Uint8 digit = 3;
	const Uint8 charHex = 4;
	const Uint8 charRest = 5;
	const Uint8 digitSign = 6;

	//array storing data about every ASCII character
	static Uint8 charDecoder[256] = { 0 };
	static bool init = true;
	if (init)
	{
		init = false;
		for(int i = 0; i < 256; ++i)
		{
			if (i == '#' || isspace(i))	charDecoder[i] = none;
			if (i == ':' || i == ';')	charDecoder[i] = spec;

			if (i == '+' || i == '-')	charDecoder[i] = digitSign;
			if (i >= '0' && i <= '9')	charDecoder[i] = digit;
			if (i >= 'A' && i <= 'F')	charDecoder[i] = charHex;
			if (i >= 'a' && i <= 'f')	charDecoder[i] = charHex;

			if (i >= 'G' && i <= 'Z')	charDecoder[i] = charRest;
			if (i >= 'g' && i <= 'z')	charDecoder[i] = charRest;
			if (i == '_' || i == '.')	charDecoder[i] = charRest;
		}
	}

	//find first nowithe character.
	bool coment = false;
	for(; _begin != _end; ++_begin)
	{
		const char c = *_begin;
		if (coment)
		{
			if (c == '\n')
			{
				coment = false;
			}
			continue;
		}
		else if (isspace(c))
		{
			continue;
		}
		else if (c == '#')
		{
			coment = true;
			continue;
		}
		break;
	}
	if (_begin == _end)
	{
		return SelectedToken{ };
	}

	auto type = TokenNone;
	auto begin = _begin;
	bool hex = false;
	int off = 0;

	for (; _begin != _end; ++_begin, ++off)
	{
		const Uint8 c = *_begin;
		const Uint8 decode = charDecoder[c];

		//end of string
		if (decode == none)
		{
			break;
		}
		else if (decode == spec)
		{
			if (c == ':')
			{
				//colon start new token, skip if we are in another one
				if (type != TokenNone)
					break;

				++_begin;
				type = excepted == TokenColon ? TokenColon : TokenInvaild;
				break;
			}
			else if (c == ';')
			{
				//semicolon start new token, skip if we are in another one
				if (type != TokenNone)
					break;
				//semicolon wait for his turn, returning empty token
				if (excepted != TokenSemicolon)
					break;

				++_begin;
				type = TokenSemicolon;
				break;
			}
			else
			{
				type = TokenInvaild;
				break;
			}
		}

		switch (type)
		{
		//begin of string
		case TokenNone:
			switch(decode)
			{
			//start of number
			case digitSign:
				--off; //skipping +- sign
			case digit:
				hex = c == '0'; //expecting hex number
				type = TokenNumber;
				continue;

			//start of symbol
			case charHex:
			case charRest:
				type = TokenSymbol;
				continue;
			}
			break;

		//middle of number
		case TokenNumber:
			switch (decode)
			{
			case charRest:
				if (off != 1) break;
				if (c != 'x' && c != 'X') break; //X in "0x1"
			case charHex:
				if (!hex) break;
			case digit:
				if (off == 0) hex = c == '0'; //expecting hex number
				continue;
			}
			break;

		//middle of symbol
		case TokenSymbol:
			switch (decode)
			{
			case charRest:
			case charHex:
			case digit:
				continue;
			}
			break;
		default:
			break;
		}
		//when decode == 0 or we find unexpected char we should end there
		type = TokenInvaild;
		break;
	}
	auto end = _begin;
	return SelectedToken{ type, ScriptRef{ begin, end } };
}

////////////////////////////////////////////////////////////
//					ParserWriter class
////////////////////////////////////////////////////////////

/**
 * Constructor
 * @param regUsed current used reg space.
 * @param c container that will be fill with script data.
 * @param d parser having all meta data.
 */
ParserWriter::ParserWriter(
		Uint8 regUsed,
		ScriptContainerBase& c,
		const ScriptParserBase& d) :
	container(c),
	parser(d),
	refListCurr(),
	refLabelsUses(),
	refLabelsList(),
	regIndexUsed(regUsed),
	constIndexUsed(-1)
{

}

/**
 * Function finalizing parsing of script
 */
void ParserWriter::relese()
{
	pushProc(Proc_exit);
	for (auto& p : refLabelsUses)
	{
		updateReserved<ProgPos>(p.first, refLabelsList[p.second]);
	}
}

/**
 * Returns reference based on name.
 * @param s name of referece.
 * @return referece data.
 */
ScriptRefData ParserWriter::getReferece(const ScriptRef& s) const
{
	auto ptr = findSortHelper(refListCurr, s);
	if (ptr == nullptr)
	{
		ptr = parser.getRef(s);
	}
	if (ptr == nullptr)
	{
		ptr = parser.getGlobal()->getRef(s);
	}
	if (ptr == nullptr)
	{
		return ScriptRefData{ };
	}
	return *ptr;
}

/**
 * Add new referece definition.
 * @param s Name of referece.
 * @param data Data of reference.
 * @return pointer to new created reference.
 */
ScriptRefData ParserWriter::addReferece(const ScriptRefData& data)
{
	addSortHelper(refListCurr, data);
	return data;
}

/**
 * Get current position in proc vector.
 * @return Position in proc vector.
 */
ProgPos ParserWriter::getCurrPos() const
{
	return static_cast<ProgPos>(container._proc.size());
}

/**
 * Get distance betwean two positions in proc vector.
 */
size_t ParserWriter::getDiffPos(ProgPos begin, ProgPos end) const
{
	if (begin > end)
	{
		throw Exception("Invaild ProgPos distance");
	}
	return static_cast<size_t>(end) - static_cast<size_t>(begin);
}

/**
 * Push zeros to fill empty space.
 * @param s Size of empty space.
 */
ProgPos ParserWriter::push(size_t s)
{
	auto curr = getCurrPos();
	container._proc.insert(container._proc.end(), s, 0);
	return static_cast<ProgPos>(curr);
}

/**
 * Update part of proc vector.
 * @param pos position to update.
 * @param data data to write.
 * @param s size of data.
 */
void ParserWriter::update(ProgPos pos, void* data, size_t s)
{
	memcpy(&container._proc[static_cast<size_t>(pos)], data, s);
}

/**
 * Push custom value on proc vector.
 * @param p pointer to value.
 * @param size size of value.
 */
void ParserWriter::pushValue(ScriptValueData v)
{
	update(push(v.size), &v.data, v.size);
}

/**
 * Pushing proc operation id on proc vector.
 */
ParserWriter::ReservedPos<ParserWriter::ProcOp> ParserWriter::pushProc(Uint8 procId)
{
	auto curr = getCurrPos();
	container._proc.push_back(procId);
	return { curr };
}

/**
 * Updating previosoly added proc operation id.
 * @param pos Position of operation.
 * @param procOffset Offset value.
 */
void ParserWriter::updateProc(ReservedPos<ProcOp> pos, int procOffset)
{
	container._proc[static_cast<size_t>(pos.getPos())] += procOffset;
}

/**
 * Try pushing label arg on proc vector. Can't use this to create loop back label.
 * @param s name of label.
 * @return true if label was succefuly added.
 */
bool ParserWriter::pushLabelTry(const ScriptRefData& data)
{
	auto temp = data;
	if (!temp && temp.name)
	{
		temp = addReferece(addLabel(data.name));
	}
	if (temp.type != ArgLabel)
	{
		return false;
	}

	auto index = temp.getValue<int>();
	if ((!data.name && refLabelsList[index] != ProgPos::Unknown))
	{
		return false;
	}
	refLabelsUses.push_back(std::make_pair(pushReserved<ProgPos>(), index));
	return true;
}

/**
 * Create new label definition for proc vector.
 * @return index of new label.
 */
ScriptRefData ParserWriter::addLabel(const ScriptRef& name)
{
	int pos = refLabelsList.size();
	refLabelsList.push_back(ProgPos::Unknown);
	return ScriptRefData{ name, ArgLabel, pos };
}

/**
 * Setting offset of label on proc vector.
 * @param s name of label
 * @param offset set value where label is pointing
 * @return true if operation success
 */
bool ParserWriter::setLabel(const ScriptRefData& data, ProgPos offset)
{
	auto temp = data;
	if (!temp && temp.name)
	{
		temp = addReferece(addLabel(data.name));
	}
	if (temp.type != ArgLabel)
	{
		return false;
	}

	auto index = temp.getValue<int>();
	if (refLabelsList[index] != ProgPos::Unknown)
	{
		return false;
	}
	refLabelsList[index] = offset;
	return true;
}

/**
 * Try pushing data arg on proc vector.
 * @param s name of data
 * @return true if data exists and is valid
 */
bool ParserWriter::pushConstTry(const ScriptRefData& data, ArgEnum type)
{
	if (data && data.type == type && !ArgIsReg(data.type) && data.value.type == type)
	{
		pushValue(data.value);
		return true;
	}
	return false;
}

/**
 * Try pushing reg arg on proc vector.
 * @param s name of reg
 * @return true if reg exists and is valid
 */
bool ParserWriter::pushRegTry(const ScriptRefData& data, ArgEnum type)
{
	type = ArgSpecAdd(type, ArgSpecReg);
	if (data && ArgCompatible(type, data.type, 0) && data.getValue<RegEnum>() != RegInvaild)
	{
		pushValue(static_cast<Uint8>(data.getValue<RegEnum>()));
		return true;
	}
	return false;
}

/**
 * Add new reg arg definition.
 * @param s name of reg
 * @param type type of reg
 * @return true if reg exists and is valid
 */
bool ParserWriter::addReg(const ScriptRef& s, ArgEnum type)
{
	ScriptRefData pos = getReferece(s);
	type = ArgSpecAdd(type, ArgSpecReg);
	if (pos)
	{
		return false;
	}
	auto size = getRegSize(parser, type);
	if (size == 0)
	{
		return false;
	}
	if (regIndexUsed + size > ScriptMaxReg)
	{
		return false;
	}
	ScriptRefData data = { s, type, static_cast<RegEnum>(regIndexUsed) };
	regIndexUsed += size;
	addReferece(data);
	return true;
}

////////////////////////////////////////////////////////////
//				ScriptParserBase class
////////////////////////////////////////////////////////////

/**
 * Constructor.
 */
ScriptParserBase::ScriptParserBase(ScriptGlobal* shared, const std::string& name) :
	_shared{ shared },
	_emptyReturn{ false },
	_regUsed{ RegMax },
	_regOutSize{ 0 }, _regOutName{ },
	_name{ name }
{
	//--------------------------------------------------
	//					op_data init
	//--------------------------------------------------
	#define MACRO_ALL_INIT(NAME, IMPL, ARGS, DESC) \
		addParserBase(#NAME, DESC, nullptr, helper::FuncGroup<MACRO_FUNC_ID(NAME)>::overloadType(), &parseBuildinProc<MACRO_PROC_ID(NAME), helper::FuncGroup<MACRO_FUNC_ID(NAME)>>, nullptr, nullptr);

	MACRO_PROC_DEFINITION(MACRO_ALL_INIT)

	#undef MACRO_ALL_INIT

	auto buildin = [&](const std::string& s, ScriptProcData::parserFunc func)
	{
		addParserBase(s, {}, &overloadBuildinProc, {}, func, nullptr, nullptr);
	};

	buildin("if", &parseIf);
	buildin("else", &parseElse);
	buildin("end", &parseEnd);
	buildin("var", &parseVar);
	buildin("debug_log", &parseDebugLog);
	buildin("loop", &parseDummy);
	buildin("break", &parseDummy);
	buildin("continue", &parseDummy);
	buildin("return", &parseReturn);

	addParser<helper::FuncGroup<Func_test_eq_null>>("test_eq", "");
	addParser<helper::FuncGroup<Func_debug_impl_int>>("debug_impl", "");
	addParser<helper::FuncGroup<Func_debug_flush>>("debug_flush", "");

	addType<int>("int");

	auto labelName = addNameRef("label");
	auto nullName = addNameRef("null");

	addSortHelper(_typeList, { labelName, ArgLabel, 0 });
	addSortHelper(_typeList, { nullName, ArgNull, 0 });
	addSortHelper(_refList, { nullName, ArgNull });
}

/**
 * Destructor
 */
ScriptParserBase::~ScriptParserBase()
{

}

/**
 * Test if that name is already in use.
 */
bool ScriptParserBase::haveNameRef(const std::string& s) const
{
	auto ref = ScriptRef{ s.data(), s.data() + s.size() };
	if (findSortHelper(_refList, ref) != nullptr)
	{
		return true;
	}
	if (findSortHelper(_procList, ref) != nullptr)
	{
		return true;
	}
	if (findSortHelper(_typeList, ref) != nullptr)
	{
		return true;
	}
	for (auto& r : ConditionNames)
	{
		if (ref == r)
		{
			return true;
		}
	}
	for (auto& r : ConditionSpecNames)
	{
		if (ref == r)
		{
			return true;
		}
	}
	return false;
}

/**
 * Strore new name reference for future use.
 */
ScriptRef ScriptParserBase::addNameRef(const std::string& s)
{
	return addString(_strings, s);
}

/**
 * Add new function parsing arguments of script operation.
 * @param s function name
 * @param parser parsing fu
 */
void ScriptParserBase::addParserBase(const std::string& s, const std::string& description, ScriptProcData::overloadFunc overload, ScriptRange<ScriptRange<ArgEnum>> overloadArg, ScriptProcData::parserFunc parser, ScriptProcData::argFunc arg, ScriptProcData::getFunc get)
{
	if (haveNameRef(s))
	{
		auto procs = getProc(ScriptRef{ s.data(), s.data() + s.size() });
		if (!procs)
		{
			throw Exception("Function name '" + s + "' already used");
		}
	}
	if (!parser)
	{
		parser = &parseCustomProc;
	}
	if (!overload)
	{
		overload = validOverloadProc(overloadArg) ? &overloadCustomProc : &overloadInvalidProc;
	}
	addSortHelper(_procList, { addNameRef(s), addNameRef(description), overload, overloadArg, parser, arg, get });
}

/**
 * Add new type to parser.
 * @param s Type name.
 * @param type
 * @param size
 */
void ScriptParserBase::addTypeBase(const std::string& s, ArgEnum type, size_t size)
{
	if (haveNameRef(s))
	{
		throw Exception("Type name '" + s + "' already used");
	}

	addSortHelper(_typeList, { addNameRef(s), ArgBase(type), size });
}

/**
 * Test if type is alredy used.
 */
bool ScriptParserBase::haveTypeBase(ArgEnum type)
{
	type = ArgBase(type);
	for (auto& v : _typeList)
	{
		if (v.type == type)
		{
			return true;
		}
	}
	return false;
}

/**
 * Set name for custom script param.
 * @param s name for custom parameter.
 * @param type type of custom parameter.
 * @param outputReg is this reg used for script output.
 */
void ScriptParserBase::addScriptReg(const std::string& s, ArgEnum type, bool writableReg, bool outputReg)
{
	if (writableReg || outputReg)
	{
		if (outputReg && _regOutSize >= ScriptMaxOut)
		{
			throw Exception("Custom output reg limit reach for: '" + s + "'");
		}
		type = ArgSpecAdd(type, ArgSpecVar);
	}
	else
	{
		type = ArgSpecAdd(ArgSpecRemove(type, ArgSpecVar), ArgSpecReg);
	}
	auto t = getType(type);
	if (t == nullptr)
	{
		throw Exception("Invalid type for reg: '" + s + "'");
	}
	auto size = getRegSize(*this, type);
	if (size == 0)
	{
		throw Exception("Invalid use of type '" + t->name.toString() + "' for reg: '" + s + "'");
	}
	if (_regUsed + size <= ScriptMaxReg)
	{
		if (haveNameRef(s))
		{
			throw Exception("Reg name '" + s + "' already used");
		}

		auto name = addNameRef(s);
		if (outputReg)
		{
			_regOutName[_regOutSize++] = name;
		}
		auto old = _regUsed;
		_regUsed += size;
		addSortHelper(_refList, { name, type, static_cast<RegEnum>(old) });
	}
	else
	{
		throw Exception("Custom reg limit reach for: '" + s + "'");
	}
}

/**
 * Add const value to script.
 * @param s name for const.
 * @param i value.
 */
void ScriptParserBase::addConst(const std::string& s, ScriptValueData i)
{
	if (haveNameRef(s))
	{
		throw Exception("Const name '" + s + "' already used");
	}

	addSortHelper(_refList, { addNameRef(s), i.type, i });
}

/**
 * Update const value in script.
 * @param s name for const.
 * @param i new value.
 */
void ScriptParserBase::updateConst(const std::string& s, ScriptValueData i)
{
	ScriptRefData* f = findSortHelper(_refList, ScriptRef{ s.data(), s.data() + s.size() });
	if (!f)
	{
		throw Exception("Unknown const with name '" + s + "' to update");
	}
	if (f->type != i.type)
	{
		throw Exception("Incompatible const with name '" + s + "' to update");
	}
	f->value = i;
}
/**
 * Get name of type
 * @param type Type id.
 */
ScriptRef ScriptParserBase::getTypeName(ArgEnum type) const
{
	auto p = getType(type);
	if (p)
	{
		return p->name;
	}
	else
	{
		return ScriptRef{ };
	}
}

/**
 * Get full name of type.
 * @param type
 * @return Full name with e.g. `var` or `ptr`.
 */
std::string ScriptParserBase::getTypePrefix(ArgEnum type) const
{
	std::string prefix;
	if (ArgIsVar(type))
	{
		prefix = "var ";
	}
	if (ArgIsPtr(type))
	{
		if (ArgIsPtrE(type))
		{
			prefix += "ptre ";
		}
		else
		{
			prefix += "ptr ";
		}
	}
	return prefix;
}

/**
 * Get type data.
 * @param type Type id.
 */
const ScriptTypeData* ScriptParserBase::getType(ArgEnum type) const
{
	ArgEnum base = ArgBase(type);
	for (auto& t : _typeList)
	{
		if (t.type == base)
		{
			return &t;
		}
	}
	return nullptr;
}

/**
 * Get type data with name equal prefix + postfix.
 * @param prefix Beginig of name.
 * @param postfix End of name.
 * @return Pointer to data or null if not find.
 */
const ScriptTypeData* ScriptParserBase::getType(ScriptRef prefix, ScriptRef postfix) const
{
	return findSortHelper(_typeList, prefix, postfix);
}

/**
 * Get function data with name equal prefix + postfix.
 * @param prefix Beginig of name.
 * @param postfix End of name.
 * @return Pointer to data or null if not find.
 */
ScriptRange<ScriptProcData> ScriptParserBase::getProc(ScriptRef prefix, ScriptRef postfix) const
{
	auto lower = _procList.data();
	auto upper = _procList.data() + _procList.size();
	lower = boundSortHelper<false>(lower, upper, prefix, postfix);
	upper = boundSortHelper<true>(lower, upper, prefix, postfix);

	return { lower, upper };
}

/**
 * Get arguments data with name equal prefix + postfix.
 * @param prefix Beginig of name.
 * @param postfix End of name.
 * @return Pointer to data or null if not find.
 */
const ScriptRefData* ScriptParserBase::getRef(ScriptRef prefix, ScriptRef postfix) const
{
	return findSortHelper(_refList, prefix, postfix);
}

/**
 * Parse string and write script to ScriptBase
 * @param src struct where final script is write to
 * @param src_code string with script
 * @return true if string have valid script
 */
bool ScriptParserBase::parseBase(ScriptContainerBase& destScript, const std::string& parentName, const std::string& srcCode) const
{
	ScriptContainerBase tempScript;
	std::string err = "Error in parsing script '" + _name + "' for '" + parentName + "': ";
	ParserWriter help(
		_regUsed,
		tempScript,
		*this
	);

	bool haveLastReturn = false;
	bool haveCodeNormal = false;
	ScriptRefTokens range = ScriptRefTokens{ srcCode.data(), srcCode.data() + srcCode.size() };
	if (!range)
	{
		return false;
	}

	while (true)
	{
		SelectedToken op = range.getNextToken();
		if (!op)
		{
			for (auto i = help.refListCurr.begin(); i != help.refListCurr.end(); ++i)
			{
				if (i->type == ArgLabel && i->value.getValue<int>() == -1)
				{
					Log(LOG_ERROR) << err << "invalid use of label: '" << i->name.toString() << "' without declaration";
					return false;
				}
			}
			if (!haveLastReturn)
			{
				Log(LOG_ERROR) << err << "script need to end with return statement";
			}
			help.relese();
			destScript = std::move(tempScript);
			return true;
		}

		auto line_begin = op.begin();
		SelectedToken label = { };
		SelectedToken args[ScriptMaxArg];
		args[0] = range.getNextToken(TokenColon);
		if (args[0].getType() == TokenColon)
		{
			std::swap(op, label);
			op = range.getNextToken();
			args[0] = range.getNextToken();
		}

		// change form of "Reg.Function" to "Type.Function Reg".
		auto op_curr = getProc(op);
		if (!op_curr)
		{
			auto first_dot = op.find('.');
			if (first_dot != std::string::npos)
			{
				auto temp = op.substr(0, first_dot);
				auto ref = help.getReferece(temp);
				if (ref && ref.type >= ArgMax)
				{
					auto name = getTypeName(ref.type);
					if (name)
					{
						op_curr = getProc(name, op.substr(first_dot));
						if (!op_curr)
						{
							Log(LOG_ERROR) << err << "invalid operation name '"<< op.toString() <<"'";
							return false;
						}
						args[1] = args[0];
						args[0] = { TokenSymbol, temp };
					}
				}
			}
		}
		for (int i = (args[1] ? 2 : 1); i < ScriptMaxArg; ++i)
			args[i] = range.getNextToken();
		SelectedToken f = range.getNextToken(TokenSemicolon);

		//validation
		bool valid = true;
		valid &= label.getType() == TokenSymbol || label.getType() == TokenNone;
		valid &= op.getType() == TokenSymbol;
		for (int i = 0; i < ScriptMaxArg; ++i)
			valid &= args[i].getType() == TokenSymbol || args[i].getType() == TokenNumber || args[i].getType() == TokenNone;
		valid &= f.getType() == TokenSemicolon;

		if (!valid)
		{
			auto line_end = range.begin();
			if (f.getType() != TokenSemicolon)
			{
				// fixing `line_end` position.
				while(line_end != range.end() && *line_end != ';')
					++line_end;
				if (line_end != range.end())
					++line_end;
			}

			if (args[ScriptMaxArg - 1].getType() != TokenNone)
			{
				Log(LOG_ERROR) << err << "too many arguments in line: '" << std::string(line_begin, line_end) << "'";
			}
			else
			{
				Log(LOG_ERROR) << err << "invalid line: '" << std::string(line_begin, line_end) << "'";
			}
			return false;
		}

		ScriptRef line = ScriptRef{ line_begin, range.begin() };
		ScriptRefData argData[ScriptMaxArg] = { };

		// test validty of operation positions
		auto isReturn = (op == ScriptRef{ "return" });
		auto isVarDef = (op == ScriptRef{ "var" });
		auto isEnd = (op == ScriptRef{ "end" }) || (op == ScriptRef{ "else" });

		if (haveLastReturn && !isEnd)
		{
			Log(LOG_ERROR) << err << "unreachable line after return: '" << line.toString() << "'";
			return false;
		}
		if (haveCodeNormal && isVarDef)
		{
			Log(LOG_ERROR) << err << "invalid variable definition after other operations: '" << line.toString() << "'";
			return false;
		}
		if (label && isVarDef)
		{
			Log(LOG_ERROR) << err << "label can't be before variable definition: '" << line.toString() << "'";
			return false;
		}

		haveLastReturn = isReturn;
		haveCodeNormal = !isVarDef;


		// matching args form operation definition with args avaliable in string
		int i = 0;
		while (i < ScriptMaxArg && args[i].getType() != TokenNone)
		{
			argData[i] = args[i].parse(help);
			++i;
		}

		if (label && !help.setLabel(label.parse(help), help.getCurrPos()))
		{
			Log(LOG_ERROR) << err << "invalid label '"<< label.toString() <<"' in line: '" << line.toString() << "'";
			return false;
		}

		// create normal proc call
		if (callOverloadProc(help, op_curr, argData, argData+i) == false)
		{
			Log(LOG_ERROR) << err << "invalid operation in line: '" << line.toString() << "'";
			return false;
		}
	}
}

/**
 * Prase string and return new script.
 */
void ScriptParserBase::parseNode(ScriptContainerBase& container, const std::string& type, const YAML::Node& node) const
{
	if(const YAML::Node& scripts = node["scripts"])
	{
		if (const YAML::Node& curr = scripts[getName()])
		{
			parseBase(container, type, curr.as<std::string>());
		}
	}
	if (!container && !getDefault().empty())
	{
		parseBase(container, type, getDefault());
	}
}

/**
 * Load global data from YAML.
 */
void ScriptParserBase::load(const YAML::Node& node)
{

}

/**
 * Print all metadata
 */
void ScriptParserBase::logScriptMetadata(bool haveEvents) const
{
	if (Options::debug)
	{
		auto argType = [&](ArgEnum type) -> std::string
		{
			return getTypeName(type).toString();
		};

		const int tabSize = 8;
		static bool printOp = true;
		if (printOp)
		{
			size_t offset = 0;
			printOp = false;
			Logger opLog;
			#define MACRO_STRCAT(...) #__VA_ARGS__
			#define MACRO_ALL_LOG(NAME, Impl, Args, Desc, ...) \
				if (validOverloadProc(helper::FuncGroup<MACRO_FUNC_ID(NAME)>::overloadType()) && strlen(Desc) != 0) opLog.get(LOG_DEBUG) \
					<< "Op:    " << std::setw(tabSize*2) << #NAME \
					<< "OpId:  " << std::setw(tabSize/2) << offset << "  + " <<  std::setw(tabSize) << helper::FuncGroup<MACRO_FUNC_ID(NAME)>::ver() \
					<< "Args:  " << std::setw(tabSize*5) << displayOverloadProc(this, helper::FuncGroup<MACRO_FUNC_ID(NAME)>::overloadType()) \
					<< "Desc:  " << Desc \
					<< "\n"; \
				offset += helper::FuncGroup<MACRO_FUNC_ID(NAME)>::ver();

			opLog.get(LOG_DEBUG) << "Available buildin script operations:\n" << std::left << std::hex << std::showbase;
			MACRO_PROC_DEFINITION(MACRO_ALL_LOG)

			#undef MACRO_ALL_LOG
			#undef MACRO_STRCAT
		}

		Logger refLog;
		refLog.get(LOG_DEBUG) << "Script info for: " << _name << "\n" << std::left;
		refLog.get(LOG_DEBUG) << "\n";
		if (haveEvents)
		{
			refLog.get(LOG_DEBUG) << "Have global events\n";
			refLog.get(LOG_DEBUG) << "\n";
		}
		if (!_defaultScript.empty())
		{
			refLog.get(LOG_DEBUG) << "Script defualt implementation:\n";
			refLog.get(LOG_DEBUG) << _defaultScript << "\n";
			refLog.get(LOG_DEBUG) << "\n";
		}
		if (_regOutSize > 0)
		{
			refLog.get(LOG_DEBUG) << "Script return values:\n";
			for (size_t i = 0; i < _regOutSize; ++i)
			{
				auto ref = getRef(_regOutName[i]);
				if (ref)
				{
					refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << ref->name.toString() << std::setw(9) << getTypePrefix(ref->type) << " " << std::setw(9) << argType(ref->type) << "\n";
				}
			}
			if (_emptyReturn)
			{
				refLog.get(LOG_DEBUG) << "In this script 'return' statment is empty, script returining values are edited directly\n";
			}
			refLog.get(LOG_DEBUG) << "\n";
		}
		refLog.get(LOG_DEBUG) << "Script data:\n";
		auto temp = _refList;
		std::sort(temp.begin(), temp.end(),
			[](const ScriptRefData& a, const ScriptRefData& b)
			{
				return std::lexicographical_compare(a.name.begin(), a.name.end(), b.name.begin(), b.name.end());
			}
		);
		for (auto& r : temp)
		{
			if (!ArgIsReg(r.type) && !ArgIsPtr(r.type) && Logger::reportingLevel() != LOG_VERBOSE)
			{
				continue;
			}
			if (ArgBase(r.type) == ArgInt && !ArgIsReg(r.type))
			{
				refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << r.name.toString() << std::setw(9) << getTypePrefix(r.type) << " " << std::setw(9) << argType(r.type) << " " << r.value.getValue<int>() << "\n";
			}
			else
			{
				refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << r.name.toString() << std::setw(9) << getTypePrefix(r.type) << " " << std::setw(9) << argType(r.type) << "\n";
			}
		}
		if (Logger::reportingLevel() != LOG_VERBOSE)
		{
			refLog.get(LOG_DEBUG) << "To see const values and custom operations use 'verboseLogging'\n";
		}
		else
		{
			auto temp = _procList;
			std::sort(temp.begin(), temp.end(),
				[](const ScriptProcData& a, const ScriptProcData& b)
				{
					return std::lexicographical_compare(a.name.begin(), a.name.end(), b.name.begin(), b.name.end());
				}
			);

			refLog.get(LOG_DEBUG) << "\n";
			refLog.get(LOG_DEBUG) << "Script operations:\n";
			for (auto& p : temp)
			{
				if (p.parserArg != nullptr && p.overloadArg && p.description.size() != 0)
				{
					refLog.get(LOG_DEBUG) << "Name: " << std::setw(40) << p.name.toString() << "Args: " << std::setw(50) << displayOverloadProc(this, p.overloadArg) << (p.description != ScriptRef{"-"} ? std::string("Desc: ") + p.description.toString() + "\n" : "\n");
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////
//				ScriptParserEventsBase class
////////////////////////////////////////////////////////////

/**
 * Constructor.
 */
ScriptParserEventsBase::ScriptParserEventsBase(ScriptGlobal* shared, const std::string& name) : ScriptParserBase(shared, name)
{
	_events.reserve(EventsMax);
	_eventsData.push_back({ 0, {} });
}

/**
 *  Prase string and return new script.
 */
void ScriptParserEventsBase::parseNode(ScriptContainerEventsBase& container, const std::string& type, const YAML::Node& node) const
{
	ScriptParserBase::parseNode(container._current, type, node);
	container._events = getEvents();
}

/**
 * Load global data from YAML.
 */
void ScriptParserEventsBase::load(const YAML::Node& node)
{
	ScriptParserBase::load(node);
	if(const YAML::Node& scripts = node["scripts"])
	{
		if (const YAML::Node& curr = scripts[getName()])
		{
			for (const YAML::Node& i : curr)
			{
				EventData data = EventData{};
				data.offset = i["offset"].as<double>(0) * OffsetScale;
				if (data.offset == 0 || data.offset >= (int)OffsetMax || data.offset <= -(int)OffsetMax)
				{
					Log(LOG_ERROR) << "Invalid offset for '" << getName() << "' equal: '" << i["offset"].as<std::string>() << "'";
					continue;
				}
				ScriptContainerBase scp;
				if (parseBase(scp, "Global Event Script", i["code"].as<std::string>("")))
				{
					data.script = std::move(scp);
					_eventsData.push_back(std::move(data));
				}
			}
		}
	}
}

/**
 * Get pointer to events.
 */
const ScriptContainerBase* ScriptParserEventsBase::getEvents() const
{
	return _events.data();
}

/**
 * Relese event data.
 */
std::vector<ScriptContainerBase> ScriptParserEventsBase::releseEvents()
{
	std::sort(std::begin(_eventsData), std::end(_eventsData), [](const EventData& a, const EventData& b) { return a.offset < b.offset; });
	for (auto& e : _eventsData)
	{
		const auto reservedSpaceForZero = e.offset < 0;
		if (_events.size() + (reservedSpaceForZero ?  2 : 1) < EventsMax)
		{
			_events.push_back(std::move(e.script));
		}
		else
		{
			Log(LOG_ERROR) << "Error in script parser '" << getName() << "': global script limit reach";
			if (reservedSpaceForZero)
			{
				_events.emplace_back();
			}
			break;
		}
	}
	_events.emplace_back();
	return std::move(_events);
}

////////////////////////////////////////////////////////////
//					ScriptValuesBase class
////////////////////////////////////////////////////////////

/**
 * Set value.
 */
void ScriptValuesBase::setBase(size_t t, int i)
{
	if (t)
	{
		if (t > values.size())
		{
			values.resize(t);
		}
		values[t - 1u] = i;
	}
}

/**
 * Get value.
 */
int ScriptValuesBase::getBase(size_t t) const
{
	if (t && t <= values.size())
	{
		return values[t - 1u];
	}
	return 0;
}

/**
 * Load values from yaml file.
 */
void ScriptValuesBase::loadBase(const YAML::Node &node, const ScriptGlobal* shared, ArgEnum type)
{
	if (const YAML::Node& tags = node["tags"])
	{
		if (tags.IsMap())
		{
			for (const std::pair<YAML::Node, YAML::Node>& pair : tags)
			{
				size_t i = shared->getTag(type, ScriptRef::tempFrom("Tag." + pair.first.as<std::string>()));
				if (i)
				{
					auto temp = 0;
					auto data = shared->getTagValueData(type, i);
					shared->getTagValueTypeData(data.valueType).load(shared, temp, pair.second);
					setBase(i, temp);
				}
			}
		}
	}
}

/**
 * Save values to yaml file.
 */
void ScriptValuesBase::saveBase(YAML::Node &node, const ScriptGlobal* shared, ArgEnum type) const
{
	YAML::Node tags;
	for (size_t i = 1; i <= values.size(); ++i)
	{
		if (int v = getBase(i))
		{
			auto temp = YAML::Node{};
			auto data = shared->getTagValueData(type, i);
			shared->getTagValueTypeData(data.valueType).save(shared, v, temp);
			tags[data.name.substr(data.name.find('.') + 1u).toString()] = temp;
		}
	}
	node["tags"] = tags;
}

////////////////////////////////////////////////////////////
//					ScriptGlobal class
////////////////////////////////////////////////////////////

/**
 * Default constructor.
 */
ScriptGlobal::ScriptGlobal()
{
	addTagValueTypeBase(
		"int",
		[](const ScriptGlobal* s, int& value, const YAML::Node& node)
		{
			if (node)
			{
				value = node.as<int>();
			}
		},
		[](const ScriptGlobal* s, const int& value, YAML::Node& node)
		{
			node = value;
		}
	);
}

/**
 * Destructor.
 */
ScriptGlobal::~ScriptGlobal()
{

}

/**
 * Get tag value.
 */
size_t ScriptGlobal::getTag(ArgEnum type, ScriptRef s) const
{
	auto data = _tagNames.find(type);
	if (data != _tagNames.end())
	{
		for (size_t i = 0; i < data->second.values.size(); ++i)
		{
			auto &name = data->second.values[i].name;
			if (name == s)
			{
				return i + 1;
			}
		}
	}
	return 0;
}

/**
 * Get name of tag value.
 */
ScriptGlobal::TagValueData ScriptGlobal::getTagValueData(ArgEnum type, size_t i) const
{
	auto data = _tagNames.find(type);
	if (data != _tagNames.end())
	{
		if (i && i <= data->second.values.size())
		{
			return data->second.values[i - 1];
		}
	}
	return {};
}

/**
 * Get tag value type data.
 */
ScriptGlobal::TagValueType ScriptGlobal::getTagValueTypeData(size_t valueType) const
{
	if (valueType < _tagValueTypes.size())
	{
		return _tagValueTypes[valueType];
	}
	return {};
}

/**
 * Get tag value type id.
 */
size_t ScriptGlobal::getTagValueTypeId(ScriptRef s) const
{
	for (size_t i = 0; i < _tagValueTypes.size(); ++i)
	{
		if (_tagValueTypes[i].name == s)
		{
			return i;
		}
	}
	return (size_t)-1;
}

/**
 * Add new tag name.
 */
size_t ScriptGlobal::addTag(ArgEnum type, ScriptRef s, size_t valueType)
{
	auto data = _tagNames.find(type);
	if (data == _tagNames.end())
	{
		throw Exception("Unknown tag type");
	}
	auto tag = getTag(type, s);
	if (tag == 0)
	{
		// is tag used for other tag type?
		if (findSortHelper(_refList, s))
		{
			return 0;
		}
		// is tag type valid?
		if (valueType >= _tagValueTypes.size())
		{
			return 0;
		}
		// test to prevent warp of index value
		if (data->second.values.size() < data->second.limit)
		{
			data->second.values.push_back(TagValueData{ s, valueType });
			addSortHelper(_refList, { s, type, data->second.crate(data->second.values.size()) });
			return data->second.values.size();
		}
		return 0;
	}
	else
	{
		return tag;
	}
}

/**
 * Strore new name reference for future use.
 */
ScriptRef ScriptGlobal::addNameRef(const std::string& s)
{
	return addString(_strings, s);
}

/**
 * Store parser.
 */
void ScriptGlobal::pushParser(ScriptParserBase* parser)
{
	parser->logScriptMetadata(false);
	_parserNames.insert(std::make_pair(parser->getName(), parser));
}

/**
 * Store parser with events.
 */
void ScriptGlobal::pushParser(ScriptParserEventsBase* parser)
{
	parser->logScriptMetadata(true);
	_parserNames.insert(std::make_pair(parser->getName(), parser));
	_parserEvents.push_back(parser);
}

/**
 * Add new const value.
 */
void ScriptGlobal::addConst(const std::string& name, ScriptValueData i)
{
	for (auto p : _parserNames)
	{
		p.second->addConst(name, i);
	}
}

/**
 * Update const value.
 */
void ScriptGlobal::updateConst(const std::string& name, ScriptValueData i)
{
	for (auto p : _parserNames)
	{
		p.second->updateConst(name, i);
	}
}

/**
 * Get global ref data.
 */
const ScriptRefData* ScriptGlobal::getRef(ScriptRef name, ScriptRef postfix) const
{
	return findSortHelper(_refList, name, postfix);
}

/**
 * Prepare for loading data.
 */
void ScriptGlobal::beginLoad()
{

}

/**
 * Finishing loading data.
 */
void ScriptGlobal::endLoad()
{
	for (auto& p : _parserEvents)
	{
		_events.push_back(p->releseEvents());
	}
	_parserNames.clear();
	_parserEvents.clear();
}

/**
 * Load global data from YAML.
 */
void ScriptGlobal::load(const YAML::Node& node)
{
	if (const YAML::Node& t = node["tags"])
	{
		for (auto& p : _tagNames)
		{
			const auto nodeName = p.second.name.toString();
			const YAML::Node &tags = t[nodeName];
			if (tags && tags.IsMap())
			{
				for (const std::pair<YAML::Node, YAML::Node>& i : tags)
				{
					auto type = i.second.as<std::string>();
					auto name = i.first.as<std::string>();
					auto invalidType = _tagValueTypes.size();
					auto valueType = invalidType;
					for (size_t t = 0; t < invalidType; ++t)
					{
						if (ScriptRef::tempFrom(type) == _tagValueTypes[t].name)
						{
							valueType = t;
							break;
						}
					}
					if (valueType != invalidType)
					{
						auto namePrefix = "Tag." + name;
						auto ref = getRef(ScriptRef::tempFrom(namePrefix));
						if (ref && ref->type != p.first)
						{
							Log(LOG_ERROR) << "Script variable '" + name + "' already used in '" + _tagNames[ref->type].name.toString() + "'.";
							continue;
						}
						auto tag = getTag(p.first, ScriptRef::tempFrom(namePrefix));
						if (tag)
						{
							auto data = getTagValueData(p.first, tag);
							if (valueType != data.valueType)
							{
								Log(LOG_ERROR) << "Script variable '" + name + "' have wrong type '" << _tagValueTypes[valueType].name.toString() << "' instead of '" << _tagValueTypes[data.valueType].name.toString() << "' in '" + nodeName + "'.";
							}
							continue;
						}
						tag = addTag(p.first, addNameRef(namePrefix), valueType);
						if (!tag)
						{
							Log(LOG_ERROR) << "Script variable '" + name + "' exceeds limit of " << (int)p.second.limit << " avaiable variables in '" + nodeName + "'.";
							continue;
						}
					}
					else
					{
						Log(LOG_ERROR) << "Invaild type def '" + type + "' for script variable '" + name + "' in '" + nodeName +"'.";
					}
				}
			}
		}
	}
	for (auto& p : _parserNames)
	{
		p.second->load(node);
	}
}

} //namespace OpenXcom
