#include "KX_MeshBuilder.h"
#include "KX_BlenderMaterial.h"
#include "KX_PyMath.h"

KX_MeshBuilderSlot::KX_MeshBuilderSlot(KX_BlenderMaterial *material, RAS_IDisplayArray::PrimitiveType primitiveType,
		const RAS_VertexFormat& format)
	:m_material(material)
{
	m_displayArray = RAS_IDisplayArray::ConstructArray(primitiveType, format);
}

KX_MeshBuilderSlot::~KX_MeshBuilderSlot()
{
	delete m_displayArray;
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
	&EXP_Value::Type,
	0, 0, 0, 0, 0, 0,
	py_base_new
};

PyMethodDef KX_MeshBuilderSlot::Methods[] = {
	{"addVertex", (PyCFunction)KX_MeshBuilderSlot::sPyAddVertex, METH_VARARGS | METH_KEYWORDS},
	{"addPrimitiveIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddPrimitiveIndex, METH_O},
	{"addTriangleIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddTriangleIndex, METH_O},
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

PyObject *KX_MeshBuilderSlot::PyAddVertex(PyObject *args, PyObject *kwds)
{
	PyObject *pypos;
	PyObject *pynormal = nullptr;
	PyObject *pytangent = nullptr;
	PyObject *pyuvs = nullptr;
	PyObject *pycolors = nullptr;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "O|OOOO:addVertex",
			{"position", "normal", "tangent", "uvs", "colors", 0},
			&pypos, &pynormal, &pytangent, &pyuvs, &pycolors))
	{
		return nullptr;
	}

	mt::vec3 pos;
	if (!PyVecTo(pypos, pos)) {
		return nullptr;
	}

	mt::vec3 normal = mt::axisZ3;
	if (pynormal && !PyVecTo(pynormal, normal)) {
		return nullptr;
	}

	mt::vec4 tangent = mt::one4;
	if (pytangent && !PyVecTo(pytangent, tangent)) {
		return nullptr;
	}

	const RAS_VertexFormat& format = m_displayArray->GetFormat();
	mt::vec2 uvs[RAS_Vertex::MAX_UNIT] = {mt::zero2};
	if (pyuvs) {
		if (!PySequence_Check(pyuvs)) {
			return nullptr;
		}

		const unsigned short size = max_ii(format.uvSize, PySequence_Size(pyuvs));
		for (unsigned short i = 0; i < size; ++i) {
			if (!PyVecTo(PySequence_GetItem(pyuvs, i), uvs[i])) {
				return nullptr;
			}
		}
	}

	unsigned int colors[RAS_Vertex::MAX_UNIT] = {0xFFFFFFFF};
	if (pycolors) {
		if (!PySequence_Check(pycolors)) {
			return nullptr;
		}

		const unsigned short size = max_ii(format.colorSize, PySequence_Size(pycolors));
		for (unsigned short i = 0; i < size; ++i) {
			mt::vec4 color;
			if (!PyVecTo(PySequence_GetItem(pycolors, i), color)) {
				return nullptr;
			}
			rgba_float_to_uchar(reinterpret_cast<unsigned char (&)[4]>(colors[i]), color.Data());
		}
	}

	RAS_Vertex vert = m_displayArray->CreateVertex(pos, uvs, tangent, colors, normal);
	m_displayArray->AddVertex(vert);
	m_displayArray->DeleteVertexData(vert);

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyAddPrimitiveIndex(PyObject *value)
{
	const int val = PyLong_AsLong(value);

	if (val < 0 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError, "expected a positive integer");
		return nullptr;
	}

	m_displayArray->AddPrimitiveIndex(val);

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyAddTriangleIndex(PyObject *value)
{
	const int val = PyLong_AsLong(value);

	if (val < 0 && PyErr_Occurred()) {
		PyErr_Format(PyExc_TypeError, "expected a positive integer");
		return nullptr;
	}

	m_displayArray->AddTriangleIndex(val);

	Py_RETURN_NONE;
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

EXP_ListValue<KX_MeshBuilderSlot>& KX_MeshBuilder::GetSlots()
{
	return m_slots;
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
	&EXP_Value::Type,
	0, 0, 0, 0, 0, 0,
	py_base_new
};

PyMethodDef KX_MeshBuilder::Methods[] = {
	{"addSlot", (PyCFunction)KX_MeshBuilder::sPyAddSlot, METH_VARARGS | METH_KEYWORDS},
	{"finish", (PyCFunction)KX_MeshBuilder::sPyFinish, METH_NOARGS},
	{nullptr, nullptr} // Sentinel
};

PyAttributeDef KX_MeshBuilder::Attributes[] = {
	EXP_PYATTRIBUTE_RO_FUNCTION("slots", KX_MeshBuilder, pyattr_get_slots),
	EXP_PYATTRIBUTE_NULL // Sentinel
};

PyObject *KX_MeshBuilder::pyattr_get_slots(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilder *self = static_cast<KX_MeshBuilder *>(self_v);
	return self->GetSlots().GetProxy();
}

PyObject *KX_MeshBuilder::PyAddSlot(PyObject *args, PyObject *kwds)
{
	PyObject *pymat;
	int primitive;
	int uvCount;
	int colorCount;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "O|iii:addSlot",
			{"material", "primitive", "uvCount", "colorCount", 0},
			&pymat, &primitive, &uvCount, &colorCount))
	{
		return nullptr;
	}

	KX_BlenderMaterial *material;
	if (!ConvertPythonToMaterial(pymat, &material, false, "meshBuilder.addSlot: material must be a KX_BlenderMaterial")) {
		return nullptr;
	}

	if (!ELEM(primitive, RAS_IDisplayArray::LINES, RAS_IDisplayArray::TRIANGLES)) {
		PyErr_SetString(PyExc_TypeError, "meshBuilder.addSlot: primitive value invalid");
		return nullptr;
	}

	if (uvCount < 1 || uvCount > 8 || colorCount < 1 || colorCount > 8) {
		PyErr_SetString(PyExc_TypeError, "meshBuilder.addSlot: uv or color count invalid, must be in range [1, 8]");
		return nullptr;
	}

	RAS_VertexFormat format{(uint8_t)uvCount, (uint8_t)colorCount};
	KX_MeshBuilderSlot *slot = new KX_MeshBuilderSlot(material, (RAS_IDisplayArray::PrimitiveType)primitive, format);
	m_slots.Add(slot);

	return slot->GetProxy();
}

PyObject *KX_MeshBuilder::PyFinish()
{
	Py_RETURN_NONE;
}

