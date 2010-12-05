/*
 * $Revision: 2027 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2010-09-01 11:55:17 +0200 (Wed, 01 Sep 2010) $
 ***************************************************************/

/** \file
 * \brief Merges nodes with neighbour to get a Multilevel Graph
 *
 * \author Gereon Bartel
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * Copyright (C). All rights reserved.
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation
 * and appearing in the files LICENSE_GPL_v2.txt and
 * LICENSE_GPL_v3.txt included in the packaging of this file.
 *
 * \par
 * In addition, as a special exception, you have permission to link
 * this software with the libraries of the COIN-OR Osi project
 * (http://www.coin-or.org/projects/Osi.xml), all libraries required
 * by Osi, and all LP-solver libraries directly supported by the
 * COIN-OR Osi project, and distribute executables, as long as
 * you follow the requirements of the GNU General Public License
 * in regard to all of the software in the executable aside from these
 * third-party libraries.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#include <ogdf/energybased/multilevelmixer/MultilevelBuilder.h>
#include <ogdf/basic/HashArray.h>

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_LOCAL_BICONNECTED_MERGER_H
#define OGDF_LOCAL_BICONNECTED_MERGER_H

namespace ogdf {

class OGDF_EXPORT LocalBiconnectedMerger : public MultilevelBuilder
{
private:
	double m_levelSizeFactor;
	NodeArray<node> m_substituteNodes;
	NodeArray<bool> m_isCut;
	HashArray<int, int> m_realNodeMarks;

	void initCuts(Graph &G);
	int realNodeMark(int index);

	bool buildOneLevel(MultilevelGraph &MLG);
	bool doMerge(MultilevelGraph &MLG, node parent, node mergePartner, int level);
	bool doMergeIfPossible(Graph &G, MultilevelGraph &MLG, node parent, node mergePartner, int level);
	bool canMerge(Graph &G, node parent, node mergePartner);
	bool canMerge(Graph &G, node parent, node mergePartner, int testStrength);

public:
	LocalBiconnectedMerger();
	void setFactor(double factor);
};

} // namespace ogdf

#endif
