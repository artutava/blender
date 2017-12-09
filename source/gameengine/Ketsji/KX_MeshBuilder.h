#ifndef __KX_MESH_BUILDER_H__
#define __KX_MESH_BUILDER_H__

#include "RAS_MeshObject.h"
#include "RAS_IDisplayArray.h"

#include "EXP_ListValue.h"

class KX_BlenderMaterial;
class KX_Scene;

class KX_MeshBuilderSlot : public EXP_Value
{
	Py_Header

private:
	KX_BlenderMaterial *m_material;
	RAS_IDisplayArray *m_displayArray;

public:
	KX_MeshBuilderSlot(KX_BlenderMaterial *material, RAS_IDisplayArray::PrimitiveType primitiveType, const RAS_VertexFormat& format);
	~KX_MeshBuilderSlot();

	virtual std::string GetName();

	KX_BlenderMaterial *GetMaterial() const;
	void SetMaterial(KX_BlenderMaterial *material);

	RAS_IDisplayArray *GetDisplayArray() const;

#ifdef WITH_PYTHON

	static PyObject *pyattr_get_material(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int pyattr_set_material(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject *pyattr_get_uvCount(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject *pyattr_get_colorCount(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject *pyattr_get_primitive(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);

	EXP_PYMETHOD(KX_MeshBuilderSlot, AddVertex);
	EXP_PYMETHOD_O(KX_MeshBuilderSlot, AddPrimitiveIndex);
	EXP_PYMETHOD_O(KX_MeshBuilderSlot, AddTriangleIndex);

#endif  // WITH_PYTHON
};

class KX_MeshBuilder : public EXP_Value
{
	Py_Header

private:
	EXP_ListValue<KX_MeshBuilderSlot> m_slots;
	RAS_MeshObject::LayersInfo m_layersInfo;
	RAS_VertexFormat m_format;

	KX_Scene *m_scene;

public:
	KX_MeshBuilder(KX_Scene *scene, const RAS_MeshObject::LayersInfo& layersInfo, const RAS_VertexFormat& format);
	~KX_MeshBuilder();

	virtual std::string GetName();

	EXP_ListValue<KX_MeshBuilderSlot>& GetSlots();

#ifdef WITH_PYTHON

	static PyObject *pyattr_get_slots(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);

	EXP_PYMETHOD(KX_MeshBuilder, AddMaterial);
	EXP_PYMETHOD_NOARGS(KX_MeshBuilder, Finish);

#endif  // WITH_PYTHON
};

#endif  // __KX_MESH_BUILDER_H__
