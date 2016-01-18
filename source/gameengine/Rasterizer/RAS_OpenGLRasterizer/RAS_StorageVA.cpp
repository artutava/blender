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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "RAS_StorageVA.h"
#include "RAS_DisplayArray.h"

#include "glew-mx.h"

#include <iostream>

RAS_DisplayList::RAS_DisplayList()
{
	for (unsigned int i = 0; i < NUM_LIST; ++i) {
		m_list[i] = -1;
	}
}

RAS_DisplayList::~RAS_DisplayList()
{
	RemoveAllList();
}

void RAS_DisplayList::RemoveAllList()
{
	for (unsigned int i = 0; i < NUM_LIST; ++i) {
		int list = m_list[i];
		if (list != -1) {
			glDeleteLists(list, 1);
		}
		m_list[i] = -1;
	}
}

void RAS_DisplayList::SetMeshModified(bool modified)
{
	if (modified) {
		RemoveAllList();
	}
}

bool RAS_DisplayList::Draw(LIST_TYPE type)
{
	int list = m_list[type];
	if (list == -1) {
		m_list[type] = list = glGenLists(1);

		glNewList(list, GL_COMPILE);

		return false;
	}

	glCallList(list);

	return true;
}

void RAS_DisplayList::End(LIST_TYPE type)
{
	glEndList();
	glCallList(m_list[type]);
}

RAS_StorageVA::RAS_StorageVA(int *texco_num, RAS_IRasterizer::TexCoGen *texco, int *attrib_num, RAS_IRasterizer::TexCoGen *attrib, int *attrib_layer) :
	m_drawingmode(RAS_IRasterizer::KX_TEXTURED),
	m_texco_num(texco_num),
	m_attrib_num(attrib_num),
	m_texco(texco),
	m_attrib(attrib),
	m_attrib_layer(attrib_layer)
{
}

RAS_StorageVA::~RAS_StorageVA()
{
}

bool RAS_StorageVA::Init()
{
	return true;
}

void RAS_StorageVA::Exit()
{
}

void RAS_StorageVA::BindPrimitives(RAS_DisplayArrayBucket *arrayBucket)
{
	RAS_DisplayArray *array = arrayBucket->GetDisplayArray();
	if (!array) {
		return;
	}

	RAS_DisplayList *displayList = GetDisplayList(arrayBucket);
	if (displayList && displayList->Draw(RAS_DisplayList::BIND_LIST)) {
		return;
	}

	static const GLsizei stride = sizeof(RAS_TexVert);
	bool wireframe = m_drawingmode <= RAS_IRasterizer::KX_WIREFRAME;
	RAS_TexVert *vertexarray = array->m_vertex.data();

	if (!wireframe)
		EnableTextures(true);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, stride, vertexarray->getXYZ());
	glNormalPointer(GL_FLOAT, stride, vertexarray->getNormal());

	if (!wireframe) {
		TexCoordPtr(vertexarray);
	}

	if (displayList) {
		displayList->End(RAS_DisplayList::BIND_LIST);
	}
}

void RAS_StorageVA::UnbindPrimitives(RAS_DisplayArrayBucket *arrayBucket)
{
	RAS_DisplayArray *array = arrayBucket->GetDisplayArray();
	if (!array) {
		return;
	}

	RAS_DisplayList *displayList = GetDisplayList(arrayBucket);
	if (displayList && displayList->Draw(RAS_DisplayList::UNBIND_LIST)) {
		return;
	}

	bool wireframe = m_drawingmode <= RAS_IRasterizer::KX_WIREFRAME;
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	if (!wireframe) {
		glDisableClientState(GL_COLOR_ARRAY);
		EnableTextures(false);
	}

	if (displayList) {
		displayList->End(RAS_DisplayList::UNBIND_LIST);
	}

}

void RAS_StorageVA::IndexPrimitives(RAS_MeshSlot *ms)
{
	RAS_DisplayArrayBucket *arrayBucket = ms->m_displayArrayBucket;

	RAS_DisplayList *displayList = GetDisplayList(arrayBucket);
	if (displayList && displayList->Draw(RAS_DisplayList::DRAW_LIST)) {
		return;
	}

	static const GLsizei stride = sizeof(RAS_TexVert);
	bool wireframe = m_drawingmode <= RAS_IRasterizer::KX_WIREFRAME;
	RAS_DisplayArray *array = ms->GetDisplayArray();
	RAS_TexVert *vertexarray = array->m_vertex.data();
	unsigned int *indexarray = array->m_index.data();
	RAS_IPolyMaterial *material = ms->m_bucket->GetPolyMaterial();

	// colors
	if (!wireframe) {
		if (material->UsesObjectColor()) {
			const MT_Vector4& rgba = ms->m_RGBAcolor;

			glDisableClientState(GL_COLOR_ARRAY);
			glColor4d(rgba[0], rgba[1], rgba[2], rgba[3]);
		}
		else {
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_UNSIGNED_BYTE, stride, vertexarray->getRGBA());
		}
	}
	else
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	// here the actual drawing takes places
	glDrawElements(GL_TRIANGLES, array->m_index.size(), GL_UNSIGNED_INT, indexarray);

	if (displayList) {
		displayList->End(RAS_DisplayList::DRAW_LIST);
	}
}

RAS_DisplayList *RAS_StorageVA::GetDisplayList(RAS_DisplayArrayBucket *arrayBucket)
{
	if (!arrayBucket->UseDisplayList()) {
		return NULL;
	}

	RAS_DisplayList *displayList = (RAS_DisplayList *)arrayBucket->GetStorageInfo();
	if (!displayList) {
		displayList = new RAS_DisplayList();
		arrayBucket->SetStorageInfo(displayList);
	}

	return displayList;
}

void RAS_StorageVA::TexCoordPtr(const RAS_TexVert *tv)
{
	/* note: this function must closely match EnableTextures to enable/disable
	 * the right arrays, otherwise coordinate and attribute pointers from other
	 * materials can still be used and cause crashes */
	int unit;

	if (GLEW_ARB_multitexture) {
		for (unit = 0; unit < *m_texco_num; unit++) {
			glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
			switch (m_texco[unit]) {
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
				{
					glTexCoordPointer(3, GL_FLOAT, sizeof(RAS_TexVert), tv->getXYZ());
					break;
				}
				case RAS_IRasterizer::RAS_TEXCO_UV:
				{
					glTexCoordPointer(2, GL_FLOAT, sizeof(RAS_TexVert), tv->getUV(unit));
					break;
				}
				case RAS_IRasterizer::RAS_TEXCO_NORM:
				{
					glTexCoordPointer(3, GL_FLOAT, sizeof(RAS_TexVert), tv->getNormal());
					break;
				}
				case RAS_IRasterizer::RAS_TEXTANGENT:
				{
					glTexCoordPointer(4, GL_FLOAT, sizeof(RAS_TexVert), tv->getTangent());
					break;
				}
				default:
					break;
			}
		}

		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}

	if (GLEW_ARB_vertex_program) {
		for (unit = 0; unit < *m_attrib_num; unit++) {
			switch (m_attrib[unit]) {
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
				{
					glVertexAttribPointerARB(unit, 3, GL_FLOAT, GL_FALSE, sizeof(RAS_TexVert), tv->getXYZ());
					break;
				}
				case RAS_IRasterizer::RAS_TEXCO_UV:
				{
					glVertexAttribPointerARB(unit, 2, GL_FLOAT, GL_FALSE, sizeof(RAS_TexVert), tv->getUV(m_attrib_layer[unit]));
					break;
				}
				case RAS_IRasterizer::RAS_TEXCO_NORM:
				{
					glVertexAttribPointerARB(unit, 3, GL_FLOAT, GL_FALSE, sizeof(RAS_TexVert), tv->getNormal());
					break;
				}
				case RAS_IRasterizer::RAS_TEXTANGENT:
				{
					glVertexAttribPointerARB(unit, 4, GL_FLOAT, GL_FALSE, sizeof(RAS_TexVert), tv->getTangent());
					break;
				}
				case RAS_IRasterizer::RAS_TEXCO_VCOL:
				{
					glVertexAttribPointerARB(unit, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(RAS_TexVert), tv->getRGBA());
					break;
				}
				default:
					break;
			}
		}
	}
}

void RAS_StorageVA::EnableTextures(bool enable)
{
	RAS_IRasterizer::TexCoGen *texco, *attrib;
	int unit, texco_num, attrib_num;

	texco = m_texco;
	texco_num = *m_texco_num;
	attrib = m_attrib;
	attrib_num = *m_attrib_num;

	if (GLEW_ARB_multitexture) {
		for (unit = 0; unit < texco_num; unit++) {
			glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);

			switch (texco[unit]) {
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
				case RAS_IRasterizer::RAS_TEXCO_UV:
				case RAS_IRasterizer::RAS_TEXCO_NORM:
				case RAS_IRasterizer::RAS_TEXTANGENT:
				{
					if (enable) {
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					}
					else {
						glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					}
					break;
				}
				default:
				{
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					break;
				}
			}
		}

		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	else {
		if (texco_num) {
			if (enable) {
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			else {
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}
	}

	if (GLEW_ARB_vertex_program) {
		for (unit = 0; unit < attrib_num; unit++) {
			switch (attrib[unit]) {
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
				case RAS_IRasterizer::RAS_TEXCO_UV:
				case RAS_IRasterizer::RAS_TEXCO_NORM:
				case RAS_IRasterizer::RAS_TEXTANGENT:
				case RAS_IRasterizer::RAS_TEXCO_VCOL:
				{
					if (enable) {
						glEnableVertexAttribArrayARB(unit);
					}
					else {
						glDisableVertexAttribArrayARB(unit);
					}
					break;
				}
				default:
				{
					glDisableVertexAttribArrayARB(unit);
					break;
				}
			}
		}
	}
}
