/*
 * Copyright (c) 2015, Chaos Software Ltd
 *
 * V-Ray For Blender
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vfb_params_json.h"

#include "vfb_node_exporter.h"
#include "vfb_utils_nodes.h"
#include "vfb_utils_blender.h"

#include <boost/format.hpp>


AttrValue DataExporter::exportLight(BL::Object ob, bool check_updated)
{
	AttrValue plugin;

	BL::Lamp lamp(ob.data());
	if (lamp) {
		PointerRNA  vrayLamp = RNA_pointer_get(&lamp.ptr, "vray");

		// Find plugin ID
		//
		std::string pluginID;

		if (lamp.type() == BL::Lamp::type_AREA) {
			pluginID = "LightRectangle";
		}
		else if (lamp.type() == BL::Lamp::type_HEMI) {
			pluginID = "LightDome";
		}
		else if (lamp.type() == BL::Lamp::type_SPOT) {
			const int spotType = RNA_enum_get(&vrayLamp, "spot_type");
			switch(spotType) {
				case 0: pluginID = "LightSpotMax"; break;
				case 1: pluginID = "LightIESMax";  break;
			}
		}
		else if (lamp.type() == BL::Lamp::type_POINT) {
			const int omniType = RNA_enum_get(&vrayLamp, "omni_type");
			switch(omniType) {
				case 0: pluginID = "LightOmniMax";    break;
				case 1: pluginID = "LightAmbientMax"; break;
				case 2: pluginID = "LightSphere";     break;
			}
		}
		else if (lamp.type() == BL::Lamp::type_SUN) {
			const int directType = RNA_enum_get(&vrayLamp, "direct_type");
			switch(directType) {
				case 0: pluginID = "LightDirectMax"; break;
				case 1: pluginID = "SunLight";       break;
			}
		}
		else {
			PRINT_ERROR("Lamp: %s Type: %i => Lamp type is not supported!",
			            ob.name().c_str(), lamp.type());
		}

		if (!pluginID.empty()) {
			PointerRNA lampPropGroup = RNA_pointer_get(&vrayLamp, pluginID.c_str());
			PluginDesc pluginDesc(getLightName(ob), pluginID);

			BL::Node     lightNode(PointerRNA_NULL);
			BL::NodeTree ntree = Nodes::GetNodeTree(lamp);
			if (ntree) {
				static boost::format  lightNodeTypeFmt("VRayNode%s");
				const std::string    &vrayNodeType = boost::str(lightNodeTypeFmt % pluginID);

				lightNode = Nodes::GetNodeByType(ntree, vrayNodeType);
			}

			if (ntree && !lightNode) {
				PRINT_ERROR("Lamp \"%s\" node tree output node is not found!",
				            ob.name().c_str());
			}

			const ParamDesc::PluginDesc &pluginParamDesc = GetPluginDescription(pluginID);
			for (const auto &descIt : pluginParamDesc.attributes) {
				const std::string         &attrName = descIt.second.name;
				const ParamDesc::AttrType &attrType = descIt.second.type;

				if (attrType > ParamDesc::AttrTypeOutputStart && attrType < ParamDesc::AttrTypeOutputEnd) {
					continue;
				}

				// For lights we are interested in mappable types only in linked sockets
				// ignore otherwize
				if (!ParamDesc::TypeHasSocket(attrType)) {
					setAttrFromPropGroup(&lampPropGroup, (ID*)lamp.ptr.data, attrName, pluginDesc);
				}
				else if (lightNode){
					BL::NodeSocket sock = Nodes::GetSocketByAttr(lightNode, attrName);
					if (sock && sock.is_linked()) {
						NodeContext context;
						AttrValue socketValue = exportSocket(ntree, sock, &context);
						if (socketValue) {
							pluginDesc.add(attrName, socketValue);
						}
					}
				}
			}

			pluginDesc.add("transform", AttrTransform(ob.matrix_world()));

			if (pluginID == "LightRectangle") {
				BL::AreaLamp areaLamp(lamp);

				const float sizeX = areaLamp.size() / 2.0f;
				const float sizeY = areaLamp.shape() == BL::AreaLamp::shape_SQUARE
				                    ? sizeX
				                    : areaLamp.size_y() / 2.0f;

				pluginDesc.add("u_size", sizeX);
				pluginDesc.add("v_size", sizeY);

				// Q: Ignoring UI option "use_rect_tex" at all?
				PluginAttr *rect_tex = pluginDesc.get("rect_tex");
				bool use_rect_tex = (rect_tex && rect_tex->attrValue.type == ValueTypePlugin);
				pluginDesc.add("use_rect_tex", use_rect_tex);
			}
			else if (pluginID == "LightDome") {
				// Q: Ignoring UI option "use_dome_tex" at all?
				PluginAttr *dome_tex = pluginDesc.get("dome_tex");
				bool use_dome_tex = (dome_tex && dome_tex->attrValue.type == ValueTypePlugin);
				pluginDesc.add("use_dome_tex", use_dome_tex);
			}
			else if (pluginID == "LightSpotMax") {
				BL::SpotLamp spotLamp(lamp);

				pluginDesc.add("fallsize", spotLamp.spot_size());
			}
			else if (ELEM(pluginID, "LightRectangle", "LightSphere", "LightDome")) {
				pluginDesc.add("objectID", ob.pass_index());
			}

			plugin = m_exporter->export_plugin(pluginDesc);
		}
	}

	return plugin;
}
