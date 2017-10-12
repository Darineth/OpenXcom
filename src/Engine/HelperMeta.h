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

#ifndef OPENXCOM_HELPERMETA_H
#define OPENXCOM_HELPERMETA_H

#include <initializer_list>

namespace OpenXcom
{

namespace helper
{


/**
 * Dummy object used to exploit argument order evaluation in std::initializer_list.
 * @param Ignored
 */
struct DummySeq
{
	DummySeq(std::initializer_list<int>) { };
};

/**
 * Tag class used for function tag dispatch based on type.
 */
template<typename T>
struct TypeTag
{
	using type = T;
};

/**
 * Tag class used for function tag dispatch based on int value.
 */
template<int I>
struct PosTag
{
	static constexpr int value = I;
};

/**
 * Tag class with list of ints.
 */
template<int... I>
struct ListTag
{
	static constexpr int size = sizeof...(I);
};

/**
 * Implemetation used to create ListTag.
 */
template<typename PT>
struct ImplMakeListTagAdd;

/**
 * Implemetation used to create ListTag.
 */
template<int... I>
struct ImplMakeListTagAdd<ListTag<I...>>
{
	using type = ListTag<I..., sizeof...(I)>;
};

/**
 * Implemetation used to create ListTag.
 */
template<int I>
struct ImplMakeListTag
{
	using type = typename ImplMakeListTagAdd<typename ImplMakeListTag<I - 1>::type>::type;
};

/**
 * Implemetation used to create ListTag.
 */
template<>
struct ImplMakeListTag<0>
{
	using type = ListTag<>;
};

/**
 * Create ListTag with template parameters starting from `0` to `I - 1`.
 */
template<int I>
using MakeListTag = typename ImplMakeListTag<I>::type;

/**
 * Remove all cv like const or volatile
 */
template<typename T>
using Decay = typename std::decay<T>::type;

/**
 * Helper used to deley static assertion error test.
 */
template<typename>
struct StaticError
{
	static constexpr bool value = false;
};


} //namespace helper

} //namespace OpenXcom


#endif /* OPENXCOM_HELPERMETA_H */

