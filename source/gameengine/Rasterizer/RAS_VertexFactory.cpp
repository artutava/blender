#include "RAS_VertexFactory.h"

#define NEW_FACTORY_UV(vertformat, uv, color) \
	if (vertformat.uvSize == uv && vertformat.colorSize == color) { \
		return new RAS_VertexFactory<RAS_VertexData<uv, color> >(); \
	}

#define NEW_FACTORY_COLOR(vertformat, color) \
	NEW_FACTORY_UV(format, 1, color); \
	NEW_FACTORY_UV(format, 2, color); \
	NEW_FACTORY_UV(format, 3, color); \
	NEW_FACTORY_UV(format, 4, color); \
	NEW_FACTORY_UV(format, 5, color); \
	NEW_FACTORY_UV(format, 6, color); \
	NEW_FACTORY_UV(format, 7, color); \
	NEW_FACTORY_UV(format, 8, color);

RAS_IVertexFactory *RAS_IVertexFactory::Construct(const RAS_VertexFormat &format)
{
	NEW_FACTORY_COLOR(format, 1);
	NEW_FACTORY_COLOR(format, 2);
	NEW_FACTORY_COLOR(format, 3);
	NEW_FACTORY_COLOR(format, 4);
	NEW_FACTORY_COLOR(format, 5);
	NEW_FACTORY_COLOR(format, 6);
	NEW_FACTORY_COLOR(format, 7);
	NEW_FACTORY_COLOR(format, 8);

	return nullptr;
}
#undef NEW_FACTORY_UV
#undef NEW_FACTORY_COLOR
