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

/** \file gameengine/Converter/KX_ConvertProperties.cpp
 *  \ingroup bgeconv
 */


#include "KX_ConvertProperties.h"


#include "DNA_object_types.h"
#include "DNA_property_types.h"
/* end of blender include block */


#include "EXP_Value.h"
#include "EXP_BoolValue.h"
#include "EXP_StringValue.h"
#include "EXP_FloatValue.h"
#include "KX_GameObject.h"
#include "EXP_IntValue.h"
#include "SCA_TimeEventManager.h"
#include "SCA_IScene.h"

#include "KX_FontObject.h"
#include "DNA_curve_types.h"

/* This little block needed for linking to Blender... */
#ifdef WIN32
#include "BLI_winstuff.h"
#endif

extern "C" {
	#include "BKE_property.h"
}

#include "CM_Message.h"

#include <sstream>
#include <fstream>

/* prototype */
void BL_ConvertTextProperty(Object* object, KX_FontObject* fontobj,SCA_TimeEventManager* timemgr,SCA_IScene* scene, bool isInActiveLayer);

void BL_ConvertProperties(Object* object,KX_GameObject* gameobj,SCA_TimeEventManager* timemgr,SCA_IScene* scene, bool isInActiveLayer)
{
	
	bProperty* prop = (bProperty*)object->prop.first;
	CValue* propval;
	bool show_debug_info;

	while (prop) {
		propval = nullptr;
		show_debug_info = bool (prop->flag & PROP_DEBUG);

		switch (prop->type) {
			case GPROP_BOOL:
			{
				propval = new CBoolValue((bool)(prop->data != 0));
				gameobj->SetProperty(prop->name,propval);
				//promp->poin= &prop->data;
				break;
			}
			case GPROP_INT:
			{
				propval = new CIntValue((int)prop->data);
				gameobj->SetProperty(prop->name,propval);
				break;
			}
			case GPROP_FLOAT:
			{
				//prop->poin= &prop->data;
				float floatprop = *((float*)&prop->data);
				propval = new CFloatValue(floatprop);
				gameobj->SetProperty(prop->name,propval);
			}
			break;
			case GPROP_STRING:
			{
				//prop->poin= callocN(MAX_PROPSTRING, "property string");
				propval = new CStringValue((char*)prop->poin,"");
				gameobj->SetProperty(prop->name,propval);
				break;
			}
			case GPROP_TIME:
			{
				float floatprop = *((float*)&prop->data);

				CValue* timeval = new CFloatValue(floatprop);
				// set a subproperty called 'timer' so that 
				// we can register the replica of this property 
				// at the time a game object is replicated (AddObjectActuator triggers this)
				CValue *bval = new CBoolValue(true);
				timeval->SetProperty("timer",bval);
				bval->Release();
				if (isInActiveLayer)
				{
					timemgr->AddTimeProperty(timeval);
				}
				
				propval = timeval;
				gameobj->SetProperty(prop->name,timeval);

			}
			default:
			{
				// todo make an assert etc.
			}
		}
		
		if (propval)
		{
			if (show_debug_info && isInActiveLayer)
			{
				scene->AddDebugProperty(gameobj,prop->name);
			}
			// done with propval, release it
			propval->Release();
		}
		
#ifdef WITH_PYTHON
		/* Warn if we double up on attributes, this isn't quite right since it wont find inherited attributes however there arnt many */
		for (PyAttributeDef *attrdef = KX_GameObject::Attributes; !attrdef->m_name.empty(); attrdef++) {
			if (prop->name == attrdef->m_name) {
				CM_Warning("user defined property name \"" << prop->name << "\" is also a python attribute for object \""
					<< object->id.name+2 << "\". Use ob[\"" << prop->name << "\"] syntax to avoid conflict");
				break;
			}
		}
		for (PyMethodDef *methdef = KX_GameObject::Methods; methdef->ml_name; methdef++) {
			if (strcmp(prop->name, methdef->ml_name)==0) {
				CM_Warning("user defined property name \"" << prop->name << "\" is also a python method for object \""
					<< object->id.name+2 << "\". Use ob[\"" << prop->name << "\"] syntax to avoid conflict");
				break;
			}
		}
		/* end warning check */
#endif // WITH_PYTHON

		prop = prop->next;
	}
	// check if state needs to be debugged
	if (object->scaflag & OB_DEBUGSTATE && isInActiveLayer)
	{
		//  reserve name for object state
		scene->AddDebugProperty(gameobj, "__state__");
	}

	/* Font Objects need to 'copy' the Font Object data body to ["Text"] */
	if (object->type == OB_FONT)
	{
		BL_ConvertTextProperty(object, (KX_FontObject *)gameobj, timemgr, scene, isInActiveLayer);
	}
}

void BL_ConvertTextProperty(Object* object, KX_FontObject* fontobj,SCA_TimeEventManager* timemgr,SCA_IScene* scene, bool isInActiveLayer)
{
	CValue* tprop = fontobj->GetProperty("Text");
	if (!tprop) return;
	bProperty* prop = BKE_bproperty_object_get(object, "Text");
	if (!prop) return;

	Curve *curve = static_cast<Curve *>(object->data);
	std::string str = curve->str;
	std::stringstream stream;
	stream << str;
	CValue* propval = nullptr;

	switch (prop->type) {
		case GPROP_BOOL:
		{
			bool value;
			stream >> value;
			propval = new CBoolValue(value != 0);
			break;
		}
		case GPROP_INT:
		{
			int value;
			stream >> value;
			propval = new CIntValue(value);
			break;
		}
		case GPROP_FLOAT:
		{
			float floatprop;
			stream >> floatprop;
			propval = new CFloatValue(floatprop);
			break;
		}
		case GPROP_STRING:
		{
			propval = new CStringValue(str, "");
			break;
		}
		case GPROP_TIME:
		{
			float floatprop;
			stream >> floatprop;

			propval = new CFloatValue(floatprop);
			// set a subproperty called 'timer' so that
			// we can register the replica of this property
			// at the time a game object is replicated (AddObjectActuator triggers this)
			CValue *bval = new CBoolValue(true);
			propval->SetProperty("timer",bval);
			bval->Release();
			if (isInActiveLayer)
			{
				timemgr->AddTimeProperty(propval);
			}
		}
		default:
		{
			BLI_assert(0);
		}
	}

	if (stream.fail() || stream.bad()) {
		CM_Error("failed convert font property \"Text\"");
	}

	if (propval) {
		tprop->SetValue(propval);
		propval->Release();
	}
}

