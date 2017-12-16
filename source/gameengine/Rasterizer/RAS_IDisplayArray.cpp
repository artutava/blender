/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Tristan Porteries.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file RAS_IDisplayArray.cpp
 *  \ingroup bgerast
 */

#include "RAS_DisplayArray.h"
#include "RAS_DisplayArrayStorage.h"
#include "RAS_MeshObject.h"

#include "CM_Template.h"

#include "GPU_glew.h"

#include <algorithm>

struct PolygonSort
{
	/// Distance from polygon center to camera near plane.
	float m_z;
	/// Index of the first vertex in the polygon.
	unsigned int m_first;

	PolygonSort() = default;

	void Init(unsigned int first, const mt::vec3& center, const mt::vec3& pnorm)
	{
		m_first = first;
		m_z = mt::dot(pnorm, center);
	}

	struct BackToFront
	{
		bool operator()(const PolygonSort &a, const PolygonSort &b) const
		{
			return a.m_z < b.m_z;
		}
	};
};

RAS_IDisplayArray::RAS_IDisplayArray(const RAS_IDisplayArray& other)
	:m_type(other.m_type),
	m_format(other.m_format),
	m_memoryFormat(other.m_memoryFormat),
	m_vertexInfos(other.m_vertexInfos),
	m_vertexDataPtrs(other.m_vertexDataPtrs),
	m_primitiveIndices(other.m_primitiveIndices),
	m_triangleIndices(other.m_triangleIndices),
	m_maxOrigIndex(other.m_maxOrigIndex),
	m_polygonCenters(other.m_polygonCenters)
{
}

RAS_IDisplayArray::RAS_IDisplayArray(PrimitiveType type, const RAS_VertexFormat& format,
		const RAS_VertexDataMemoryFormat& memoryFormat)
	:m_type(type),
	m_format(format),
	m_memoryFormat(memoryFormat),
	m_maxOrigIndex(0)
{
}

RAS_IDisplayArray::RAS_IDisplayArray(PrimitiveType type, const RAS_VertexFormat& format,
		const RAS_VertexDataMemoryFormat& memoryFormat, const IndexList& primitiveIndices, const IndexList& triangleIndices)
	:m_type(type),
	m_format(format),
	m_memoryFormat(memoryFormat),
	m_primitiveIndices(primitiveIndices),
	m_triangleIndices(triangleIndices),
	m_maxOrigIndex(0)
{
}

RAS_IDisplayArray::~RAS_IDisplayArray()
{
}

RAS_IDisplayArray *RAS_IDisplayArray::Construct(RAS_IDisplayArray::PrimitiveType type, const RAS_VertexFormat &format)
{
	return CM_InstantiateTemplateSwitch<RAS_IDisplayArray, RAS_DisplayArray, RAS_VertexFormatTuple>(format, type, format);
}

RAS_IDisplayArray *RAS_IDisplayArray::Construct(RAS_IDisplayArray::PrimitiveType type, const RAS_VertexFormat &format,
		const IVertexDataList& vertices, const IndexList& primitiveIndices, const IndexList& triangleIndices)
{
	return CM_InstantiateTemplateSwitch<RAS_IDisplayArray, RAS_DisplayArray, RAS_VertexFormatTuple>(format,
			type, format, vertices, primitiveIndices, triangleIndices);
}

void RAS_IDisplayArray::SortPolygons(const mt::mat3x4& transform, unsigned int *indexmap)
{
	const unsigned int totpoly = GetPrimitiveIndexCount() / 3;

	if (totpoly <= 1 || m_type == LINES) {
		return;
	}

	// Extract camera Z plane.
	const mt::vec3 pnorm(transform[2], transform[5], transform[8]);

	if (m_polygonCenters.size() != totpoly) {
		m_polygonCenters.resize(totpoly, mt::zero3);
		for (unsigned int i = 0; i < totpoly; ++i) {
			// Compute polygon center.
			mt::vec3& center = m_polygonCenters[i];
			for (unsigned short j = 0; j < 3; ++j) {
				/* Note that we don't divide by 3 as it is not needed
				 * to compare polygons. */
				center += GetVertex(m_primitiveIndices[i * 3 + j]).xyz();
			}
		}
	}

	std::vector<PolygonSort> sortedPoly(totpoly);
	// Get indices and polygon distance into temporary array.
	for (unsigned int i = 0; i < totpoly; ++i) {
		sortedPoly[i].Init(i * 3, pnorm, m_polygonCenters[i]);
	}

	std::sort(sortedPoly.begin(), sortedPoly.end(), PolygonSort::BackToFront());

	// Get indices from temporary array.
	for (unsigned int i = 0; i < totpoly; ++i) {
		const unsigned int first = sortedPoly[i].m_first;
		for (unsigned short j = 0; j < 3; ++j) {
			indexmap[i * 3 + j] = m_primitiveIndices[first + j];
		}
	}
}

void RAS_IDisplayArray::InvalidatePolygonCenters()
{
	m_polygonCenters.clear();
}

RAS_IDisplayArray::PrimitiveType RAS_IDisplayArray::GetPrimitiveType() const
{
	return m_type;
}

int RAS_IDisplayArray::GetOpenGLPrimitiveType() const
{
	switch (m_type) {
		case LINES:
		{
			return GL_LINES;
		}
		case TRIANGLES:
		{
			return GL_TRIANGLES;
		}
	}
	return 0;
}

void RAS_IDisplayArray::UpdateFrom(RAS_IDisplayArray *other, int flag)
{
	BLI_assert(m_format == other->GetFormat());

	if (flag & TANGENT_MODIFIED) {
		for (unsigned int i = 0, size = other->GetVertexCount(); i < size; ++i) {
			GetVertex(i).SetTangent(other->GetVertex(i).GetTangent());
		}
	}
	if (flag & UVS_MODIFIED) {
		const unsigned short uvSize = m_format.uvSize;
		for (unsigned int i = 0, size = other->GetVertexCount(); i < size; ++i) {
			for (unsigned int uv = 0; uv < uvSize; ++uv) {
				GetVertex(i).SetUV(uv, other->GetVertex(i).GetUv(uv));
			}
		}
	}
	if (flag & POSITION_MODIFIED) {
		for (unsigned int i = 0, size = other->GetVertexCount(); i < size; ++i) {
			GetVertex(i).SetXYZ(other->GetVertex(i).GetXYZ());
		}
	}
	if (flag & NORMAL_MODIFIED) {
		for (unsigned int i = 0, size = other->GetVertexCount(); i < size; ++i) {
			GetVertex(i).SetNormal(other->GetVertex(i).GetNormal());
		}
	}
	if (flag & COLORS_MODIFIED) {
		const unsigned short colorSize = m_format.colorSize;
		for (unsigned int i = 0, size = other->GetVertexCount(); i < size; ++i) {
			for (unsigned int color = 0; color < colorSize; ++color) {
				GetVertex(i).SetColor(color, other->GetVertex(i).GetRawColor(color));
			}
		}
	}
}

const RAS_VertexFormat& RAS_IDisplayArray::GetFormat() const
{
	return m_format;
}

const RAS_VertexDataMemoryFormat& RAS_IDisplayArray::GetMemoryFormat() const
{
	return m_memoryFormat;
}

RAS_IDisplayArray::Type RAS_IDisplayArray::GetType() const
{
	return NORMAL;
}

RAS_DisplayArrayStorage *RAS_IDisplayArray::GetStorage()
{
	return &m_storage;
}

void RAS_IDisplayArray::ConstructStorage()
{
	m_storage.Construct(this);
	m_storage.UpdateSize();
}
