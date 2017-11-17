#ifndef __KX_MESH_BUILDER_H__
#define __KX_MESH_BUILDER_H__

#include "EXP_Value.h"

#include "RAS_IDisplayArray.h"

class KX_BlenderMaterial;

class KX_MeshBuilderSlot : public EXP_Value
{
	Py_Header

private:
	KX_BlenderMaterial *m_material;
	RAS_IDisplayArray *m_displayArray;

public:
	KX_MeshBuilderSlot(KX_BlenderMaterial *material, RAS_IDisplayArray *array);
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

	EXP_PYMETHOD(KX_MeshBuilderSlot, addVertex);
	EXP_PYMETHOD_O(KX_MeshBuilderSlot, addIndex);

#endif  // WITH_PYTHON
};

class KX_MeshBuilder : public EXP_Value
{
private:
	std::vector<KX_MeshBuilderSlot *> m_slots;

public:
	KX_MeshBuilder();
	~KX_MeshBuilder();

	virtual std::string GetName();

#ifdef WITH_PYTHON

	static PyObject *pyattr_get_slots(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);

	EXP_PYMETHOD(KX_MeshBuilder, addSlot);
	EXP_PYMETHOD_NOARGS(KX_MeshBuilder, finish);

#endif  // WITH_PYTHON
};

#endif  // __KX_MESH_BUILDER_H__
