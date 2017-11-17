#include "KX_MeshBuilder.h"
#include "KX_BlenderMaterial.h"

KX_MeshBuilderSlot::KX_MeshBuilderSlot(KX_BlenderMaterial *material, RAS_IDisplayArray *array)
	:m_material(material),
	m_displayArray(array)
{
}

KX_MeshBuilderSlot::~KX_MeshBuilderSlot()
{
}

std::string KX_MeshBuilderSlot::GetName()
{
	return m_material->GetName();
}

KX_BlenderMaterial *KX_MeshBuilderSlot::GetMaterial() const
{
	return m_material;
}

void KX_MeshBuilderSlot::SetMaterial(KX_BlenderMaterial *material)
{
	m_material = material;
}

RAS_IDisplayArray *KX_MeshBuilderSlot::GetDisplayArray() const
{
	return m_displayArray;
}

PyTypeObject KX_MeshBuilderSlot::Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"KX_MeshBuilderSlot",
	sizeof(EXP_PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0, 0, 0, 0, 0, 0, 0,
	Methods,
	0,
	0,
	&BL_Shader::Type,
	0, 0, 0, 0, 0, 0,
	py_base_new
};

PyMethodDef KX_MeshBuilderSlot::Methods[] = {
	EXP_PYMETHODTABLE_KEYWORDS(KX_MeshBuilderSlot, addVertex),
	EXP_PYMETHODTABLE_O(KX_MeshBuilderSlot, addIndex),
	{nullptr, nullptr} // Sentinel
};

PyAttributeDef KX_MeshBuilderSlot::Attributes[] = {
	EXP_PYATTRIBUTE_RW_FUNCTION("material", KX_MeshBuilderSlot, pyattr_get_material, pyattr_set_material),
	EXP_PYATTRIBUTE_RO_FUNCTION("uvCount", KX_MeshBuilderSlot, pyattr_get_uvCount),
	EXP_PYATTRIBUTE_RO_FUNCTION("colorCount", KX_MeshBuilderSlot, pyattr_get_colorCount),
	EXP_PYATTRIBUTE_RO_FUNCTION("primitive", KX_MeshBuilderSlot, pyattr_get_primitive),
	EXP_PYATTRIBUTE_NULL // Sentinel
};

PyObject *KX_MeshBuilderSlot::pyattr_get_material(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return self->GetMaterial()->GetProxy();
}

int KX_MeshBuilderSlot::pyattr_set_material(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	KX_BlenderMaterial *mat;
	if (ConvertPythonToMaterial(value, &mat, false, "slot.material = material; KX_MeshBuilderSlot excepted a KX_BlenderMaterial.")) {
		return PY_SET_ATTR_FAIL;
	}

	self->SetMaterial(mat);
	return PY_SET_ATTR_SUCCESS;
}

PyObject *KX_MeshBuilderSlot::pyattr_get_uvCount(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return PyBool_FromLong(self->GetDisplayArray()->GetFormat().uvSize);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_colorCount(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return PyBool_FromLong(self->GetDisplayArray()->GetFormat().colorSize);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_primitive(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return PyBool_FromLong(self->GetDisplayArray()->GetPrimitiveType());
}

EXP_PYMETHODDEF(KX_MeshBuilderSlot, addVertex, " addOffScreen(slots, depth, width, height, hdr,  mipmap)")
{
	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "i|iiiii:addOffScreen",
			{"slots", "depth", "width", "height", "hdr", "mipmap", 0},
			&slots, &depth, &width, &height, &hdr, &mipmap))
	{
		return nullptr;
	}
}

EXP_PYMETHODDEF_O(KX_MeshBuilderSlot, addIndex, " addOffScreen(slots, depth, width, height, hdr,  mipmap)")
{
}

KX_MeshBuilder::KX_MeshBuilder()
{
}

KX_MeshBuilder::~KX_MeshBuilder()
{
}

std::string KX_MeshBuilder::GetName()
{
	return "KX_MeshBuilder";
}

PyTypeObject KX_MeshBuilder::Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"KX_MeshBuilder",
	sizeof(EXP_PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0, 0, 0, 0, 0, 0, 0,
	Methods,
	0,
	0,
	&BL_Shader::Type,
	0, 0, 0, 0, 0, 0,
	py_base_new
};

PyMethodDef KX_MeshBuilder::Methods[] = {
	EXP_PYMETHODTABLE(KX_MeshBuilder, addSlot),
	{nullptr, nullptr} // Sentinel
};

PyAttributeDef KX_MeshBuilder::Attributes[] = {
	EXP_PYATTRIBUTE_RO_FUNCTION("slots", KX_MeshBuilder, pyattr_get_slots),
	EXP_PYATTRIBUTE_NULL // Sentinel
};

