#include "KX_MeshBuilder.h"
#include "KX_MeshProxy.h"
#include "KX_BlenderMaterial.h"
#include "KX_Scene.h"
#include "KX_KetsjiEngine.h"
#include "KX_Globals.h"
#include "KX_PyMath.h"

#include "BL_BlenderConverter.h"

#include "RAS_BucketManager.h"

KX_MeshBuilderSlot::KX_MeshBuilderSlot(KX_BlenderMaterial *material, RAS_IDisplayArray::PrimitiveType primitiveType,
		const RAS_VertexFormat& format)
	:m_material(material),
	m_primitive(primitiveType),
	m_factory(RAS_IVertexFactory::Construct(format))
{
}

KX_MeshBuilderSlot::~KX_MeshBuilderSlot()
{
}

std::string KX_MeshBuilderSlot::GetName()
{
	return m_material->GetName().substr(2);
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
	{"removeVertex", (PyCFunction)KX_MeshBuilderSlot::sPyRemoveVertex, METH_VARARGS},
	{"addPrimitiveIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddPrimitiveIndex, METH_O},
	{"removePrimitiveIndex", (PyCFunction)KX_MeshBuilderSlot::sPyRemovePrimitiveIndex, METH_VARARGS},
	{"addTriangleIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddTriangleIndex, METH_O},
	{"removeTriangleIndex", (PyCFunction)KX_MeshBuilderSlot::sPyRemoveTriangleIndex, METH_VARARGS},
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
	const unsigned int index = m_displayArray->AddVertex(vert);
	m_displayArray->DeleteVertexData(vert);

	return PyLong_FromLong(index);
}

PyObject *KX_MeshBuilderSlot::PyAddPrimitiveIndex(PyObject *value)
{
	if (!PySequence_Check(value)) {
		PyErr_Format(PyExc_TypeError, "expected a list");
		return nullptr;
	}

	for (unsigned int i = 0, size = PySequence_Size(value); i < size; ++i) {
		const int val = PyLong_AsLong(PySequence_GetItem(value, i));

		if (val < 0 && PyErr_Occurred()) {
			PyErr_Format(PyExc_TypeError, "expected a list of positive integer");
			return nullptr;
		}

		m_displayArray->AddPrimitiveIndex(val);
	}

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyAddTriangleIndex(PyObject *value)
{
	if (!PySequence_Check(value)) {
		PyErr_Format(PyExc_TypeError, "expected a list");
		return nullptr;
	}

	for (unsigned int i = 0, size = PySequence_Size(value); i < size; ++i) {
		const int val = PyLong_AsLong(PySequence_GetItem(value, i));

		if (val < 0 && PyErr_Occurred()) {
			PyErr_Format(PyExc_TypeError, "expected a list of positive integer");
			return nullptr;
		}

		m_displayArray->AddTriangleIndex(val);
	}

	Py_RETURN_NONE;
}

KX_MeshBuilder::KX_MeshBuilder(KX_Scene *scene, const RAS_MeshObject::LayersInfo& layersInfo, const RAS_VertexFormat& format)
	:m_layersInfo(layersInfo),
	m_format(format),
	m_scene(scene)
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

static bool convertPythonListToLayers(PyObject *list, RAS_MeshObject::LayerList& layers, const std::string& errmsg)
{
	if (!PySequence_Check(list)) {
		PyErr_Format(PyExc_TypeError, "%s expected a list", errmsg.c_str());
		return false;
	}

	const unsigned short size = PySequence_Size(list);
	if (size > RAS_Vertex::MAX_UNIT) {
		PyErr_Format(PyExc_TypeError, "%s excepted a list of maximum %i items", errmsg.c_str(), RAS_Vertex::MAX_UNIT);
		return false;
	}

	for (unsigned short i = 0; i < size; ++i) {
		PyObject *value = PySequence_GetItem(list, i);

		if (!PyUnicode_Check(value)) {
			PyErr_Format(PyExc_TypeError, "%s excepted a list of string", errmsg.c_str());
		}

		const std::string name = std::string(_PyUnicode_AsString(value));
		layers.push_back({i, name});
	}

	return true;
}

static PyObject *py_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *pyscene;
	PyObject *pyuvs;
	PyObject *pycolors;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "O|OO:KX_MeshBuilder",
			{"scene", "uvs", "colors", 0}, &pyscene, &pyuvs, &pycolors))
	{
		return nullptr;
	}

	KX_Scene *scene;
	if (!ConvertPythonToScene(pyscene, &scene, false, "KX_MeshBuilder(scene, uvs, colors): scene must be KX_Scene")) {
		return nullptr;
	}

	RAS_MeshObject::LayersInfo layersInfo;
	if (!convertPythonListToLayers(pyuvs, layersInfo.uvLayers, "KX_MeshBuilder(scene, uvs, colors): uvs:") ||
		!convertPythonListToLayers(pycolors, layersInfo.colorLayers, "KX_MeshBuilder(scene, uvs, colors): colors:"))
	{
		return nullptr;
	}

	RAS_VertexFormat format{(uint8_t)layersInfo.uvLayers.size(), (uint8_t)layersInfo.colorLayers.size()};

	KX_MeshBuilder *builder = new KX_MeshBuilder(scene, layersInfo, format);

	return builder->NewProxy(true);
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
	py_new
};

PyMethodDef KX_MeshBuilder::Methods[] = {
	{"addMaterial", (PyCFunction)KX_MeshBuilder::sPyAddMaterial, METH_VARARGS | METH_KEYWORDS}, // TODO slot/material ?
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

PyObject *KX_MeshBuilder::PyAddMaterial(PyObject *args, PyObject *kwds)
{
	PyObject *pymat;
	int primitive;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "O|i:addMaterial", {"material", "primitive", 0}, &pymat, &primitive)) {
		return nullptr;
	}

	KX_BlenderMaterial *material;
	if (!ConvertPythonToMaterial(pymat, &material, false, "meshBuilder.addMaterial(...): material must be a KX_BlenderMaterial")) {
		return nullptr;
	}

	if (!ELEM(primitive, RAS_IDisplayArray::LINES, RAS_IDisplayArray::TRIANGLES)) {
		PyErr_SetString(PyExc_TypeError, "meshBuilder.addMaterial(...): primitive value invalid");
		return nullptr;
	}

	KX_MeshBuilderSlot *slot = new KX_MeshBuilderSlot(material, (RAS_IDisplayArray::PrimitiveType)primitive, m_format);
	m_slots.Add(slot);

	return slot->GetProxy();
}

PyObject *KX_MeshBuilder::PyFinish()
{
	if (m_slots.GetCount() == 0) {
		PyErr_SetString(PyExc_TypeError, "meshBuilder.finish(): no mesh data found");
		return nullptr;
	}

	// TODO name
	RAS_MeshObject *mesh = new RAS_MeshObject("MeshBuilder", m_layersInfo);

	RAS_BucketManager *bucketManager = m_scene->GetBucketManager();
	for (unsigned short i = 0, size = m_slots.GetCount(); i < size; ++i) {
		KX_MeshBuilderSlot *slot = m_slots.GetValue(i);
		bool created;
		RAS_MaterialBucket *bucket = bucketManager->FindBucket(slot->GetMaterial(), created);
		mesh->AddMaterial(bucket, i, slot->GetDisplayArray()->GetReplica());
	}

	mesh->EndConversion(m_scene->GetBoundingBoxManager());

	KX_GetActiveEngine()->GetConverter()->RegisterMesh(m_scene, mesh);

	return (new KX_MeshProxy(mesh))->NewProxy(true);
}

