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

/** \file RAS_Vertex.h
 *  \ingroup bgerast
 */

#ifndef __RAS_TEXVERT_H__
#define __RAS_TEXVERT_H__

#include "RAS_VertexData.h"
#include "RAS_VertexFormat.h"

#include "mathfu.h"

#include "BLI_math_vector.h"

class RAS_VertexInfo
{
public:
	enum {
		FLAT = 1,
	};

private:
	unsigned int m_origindex;
	short m_softBodyIndex;
	uint8_t m_flag;

public:
	RAS_VertexInfo(unsigned int origindex, bool flat);
	~RAS_VertexInfo();

	inline const unsigned int GetOrigIndex() const
	{
		return m_origindex;
	}

	inline short int GetSoftBodyIndex() const
	{
		return m_softBodyIndex;
	}

	inline void SetSoftBodyIndex(short int sbIndex)
	{
		m_softBodyIndex = sbIndex;
	}

	inline const uint8_t GetFlag() const
	{
		return m_flag;
	}

	inline void SetFlag(const uint8_t flag)
	{
		m_flag = flag;
	}
};

class RAS_Vertex
{
public:
	enum {
		MAX_UNIT = 8
	};

private:
	RAS_IVertexData *m_data;
	RAS_VertexFormat m_format;

public:
	RAS_Vertex(RAS_IVertexData *data, const RAS_VertexFormat& format)
		:m_data(data),
		m_format(format)
	{
	}

	~RAS_Vertex()
	{
	}

	inline RAS_IVertexData *GetData() const
	{
		return m_data;
	}

	inline const RAS_VertexFormat& GetFormat() const
	{
		return m_format;
	}

	inline const float (&GetXYZ() const)[3]
	{
		return m_data->position;
	}

	inline const float (&GetNormal() const)[3]
	{
		return m_data->normal;
	}

	inline const float (&GetTangent() const)[4]
	{
		return m_data->tangent;
	}

	inline mt::vec3 xyz() const
	{
		return mt::vec3(m_data->position);
	}

	inline void SetXYZ(const mt::vec3& xyz)
	{
		xyz.Pack(m_data->position);
	}

	inline void SetXYZ(const float xyz[3])
	{
		copy_v3_v3(m_data->position, xyz);
	}

	inline void SetNormal(const mt::vec3& normal)
	{
		normal.Pack(m_data->normal);
	}

	inline void SetNormal(const float normal[3])
	{
		copy_v3_v3(m_data->normal, normal);
	}

	inline void SetTangent(const mt::vec4& tangent)
	{
		tangent.Pack(m_data->tangent);
	}

	inline void SetTangent(const float tangent[4])
	{
		copy_v4_v4(m_data->tangent, tangent);
	}

	inline const float (&GetUv(const unsigned short index) const)[2]
	{
		return m_data->GetUv(index, m_format);
	}

	inline void SetUV(const unsigned short index, const mt::vec2& uv)
	{
		uv.Pack(m_data->GetUv(index, m_format));
	}

	inline void SetUV(const unsigned short index, const float uv[2])
	{
		copy_v2_v2(m_data->GetUv(index, m_format), uv);
	}

	inline const unsigned char (&GetColor(const unsigned short index) const)[4]
	{
		return reinterpret_cast<const unsigned char (&)[4]>(m_data->GetColor(index, m_format));
	}

	inline const unsigned int GetRawColor(const unsigned short index) const
	{
		return m_data->GetColor(index, m_format);
	}

	inline void SetColor(const unsigned short index, const unsigned int color)
	{
		m_data->GetColor(index, m_format) = color;
	}

	inline void SetColor(const unsigned short index, const mt::vec4& color)
	{
		unsigned char (&colp)[4] = reinterpret_cast<unsigned char (&)[4]>(m_data->GetColor(index, m_format));
		colp[0] = (unsigned char)(color[0] * 255.0f);
		colp[1] = (unsigned char)(color[1] * 255.0f);
		colp[2] = (unsigned char)(color[2] * 255.0f);
		colp[3] = (unsigned char)(color[3] * 255.0f);
	}

	inline void Transform(const mt::mat4& mat, const mt::mat4& nmat)
	{
		SetXYZ(mat * mt::vec3(m_data->position));
		SetNormal(nmat * mt::vec3(m_data->normal));
		SetTangent(nmat * mt::vec4(m_data->tangent));
	}

	inline void TransformUv(const int index, const mt::mat4& mat)
	{
		SetUV(index, (mat * mt::vec3(GetUv(index)[0], GetUv(index)[1], 0.0f)).xy());
	}
};

#endif  // __RAS_TEXVERT_H__
