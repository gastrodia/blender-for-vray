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

#include "vfb_node_exporter.h"
#include "vfb_utils_blender.h"
#include "vfb_utils_object.h"
#include "vfb_utils_nodes.h"


AttrValue DataExporter::exportObject(BL::Object ob, bool check_updated, const ObjectOverridesAttrs & override)
{
	AttrPlugin  node;

	BL::ID data(ob.data());
	if (data) {
		AttrPlugin  geom;
		AttrPlugin  mtl;

		bool is_updated      = check_updated ? ob.is_updated()      : true;
		bool is_data_updated = check_updated ? ob.is_updated_data() : true;

		if (!is_updated && ob.parent()) {
			BL::Object parent(ob.parent());
			is_updated = parent.is_updated();
		}
		if (!is_data_updated && ob.parent()) {
			BL::Object parent(ob.parent());
			is_data_updated = parent.is_updated_data();
		}

		BL::NodeTree ntree = Nodes::GetNodeTree(ob);
		if (ntree) {
			is_data_updated |= ntree.is_updated();
			DataExporter::tag_ntree(ntree, false);
		}

		bool isMeshLight = false;

		// XXX: Check for valid mesh?

		if (!ntree) {
			if (!(is_data_updated) || !(m_settings.export_meshes)) {
				geom = AttrPlugin(getMeshName(ob));
			}
			else {
				geom = exportGeomStaticMesh(ob);
				if (!geom) {
					PRINT_ERROR("Object: %s => Incorrect geometry!",
						ob.name().c_str());
				}
			}
			if (is_updated) {
				// NOTE: It's easier just to reexport full material
				mtl = exportMtlMulti(ob);
			}
		}
		else {
			// Export object data from node tree
			//
			BL::Node nodeOutput = Nodes::GetNodeByType(ntree, "VRayNodeObjectOutput");
			if (!nodeOutput) {
				PRINT_ERROR("Object: %s Node tree: %s => Output node not found!",
					ob.name().c_str(), ntree.name().c_str());
			} else {
				BL::NodeSocket geometrySocket = Nodes::GetInputSocketByName(nodeOutput, "Geometry");
				if (!(geometrySocket && geometrySocket.is_linked())) {
					PRINT_ERROR("Object: %s Node tree: %s => Geometry node is not set!",
						ob.name().c_str(), ntree.name().c_str());
				} else {
					NodeContext context(m_data, m_scene, ob);

					geom = DataExporter::exportSocket(ntree, geometrySocket, context);
					if (!geom) {
						PRINT_ERROR("Object: %s Node tree: %s => Incorrect geometry!",
							ob.name().c_str(), ntree.name().c_str());
					} else {
						BL::Node geometryNode = DataExporter::getConnectedNode(geometrySocket, context);

						isMeshLight = geometryNode.bl_idname() == "VRayNodeLightMesh";

						// Check if connected node is a LightMesh,
						// if so there is no need to export materials
						if (isMeshLight) {
							// Add LightMesh plugin to plugins generated by current object
							m_id_track.insert(ob, geom.plugin);
						} else {
							BL::NodeSocket materialSocket = Nodes::GetInputSocketByName(nodeOutput, "Material");
							if (!(materialSocket && materialSocket.is_linked())) {
								PRINT_ERROR("Object: %s Node tree: %s => Material node is not set! Using object materials.",
									ob.name().c_str(), ntree.name().c_str());

								// Use existing object materials
								mtl = exportMtlMulti(ob);
							} else {
								mtl = DataExporter::exportSocket(ntree, materialSocket, context);
								if (!mtl) {
									PRINT_ERROR("Object: %s Node tree: %s => Incorrect material!",
										ob.name().c_str(), ntree.name().c_str());
								}
							}
						}
					}
				}
			}
		}

		const std::string &nodePluginName = getNodeName(ob);

		// Add Node plugin to plugins generated by current object
		m_id_track.insert(ob, nodePluginName);

		// If no material is generated use default or override
		if (!mtl) {
			mtl = m_defaults.override_material
			      ? m_defaults.override_material.valPlugin
			      : m_defaults.default_material.valPlugin;
		}

		if (geom && mtl && (is_updated || is_data_updated)) {
			// No need to export Node if the object is LightMesh
			if (!isMeshLight) {
				const std::string & exportName = override ? override.namePrefix + nodePluginName : nodePluginName;

				PluginDesc nodeDesc(exportName, "Node");
				nodeDesc.add("geometry", geom);
				nodeDesc.add("material", mtl);
				nodeDesc.add("objectID", ob.pass_index());
				if (override) {
					nodeDesc.add("visible", override.visible);
					nodeDesc.add("transform", override.tm);
				} else {
					nodeDesc.add("transform", AttrTransformFromBlTransform(ob.matrix_world()));
					nodeDesc.add("visible", ob.is_visible(m_scene));
				}

				node = m_exporter->export_plugin(nodeDesc);
			}
		}
	}

	return node;
}


void DataExporter::exportHair(BL::Object ob, BL::ParticleSystemModifier psm, BL::ParticleSystem psys, bool check_updated)
{
	bool is_updated      = check_updated ? ob.is_updated()      : true;
	bool is_data_updated = check_updated ? ob.is_updated_data() : true;

	BL::ParticleSettings pset(psys.settings());
	if (pset && (pset.type() == BL::ParticleSettings::type_HAIR) && (pset.render_type() == BL::ParticleSettings::render_type_PATH)) {
		const int hair_is_updated = check_updated
						            ? (is_updated || pset.is_updated())
						            : true;

		const int hair_is_data_updated = check_updated
						                 ? (is_data_updated || pset.is_updated())
						                 : true;

		const std::string hairNodeName = "Node@" + getHairName(ob, psys, pset);

		// Put hair node to the object dependent plugines
		// (will be used to remove plugin when object is removed)
		m_id_track.insert(ob, hairNodeName);

		AttrValue hair_geom;
		if (!(hair_is_data_updated) || !(m_settings.export_meshes)) {
			hair_geom = AttrPlugin(getHairName(ob, psys, pset));
		}
		else {
			hair_geom = exportGeomMayaHair(ob, psys, psm);;
		}

		AttrValue hair_mtl;
		const int hair_mtl_index = pset.material() - 1;
		if (ob.material_slots.length() && (hair_mtl_index < ob.material_slots.length())) {
			BL::Material hair_material = ob.material_slots[hair_mtl_index].material();
			if (hair_material) {
				hair_mtl = exportMaterial(hair_material, true);
			}
		}
		if (!hair_mtl) {
			hair_mtl = m_defaults.override_material
						? m_defaults.override_material.valPlugin
						: m_defaults.default_material.valPlugin;
		}

		if (hair_geom && hair_mtl && (hair_is_updated || hair_is_data_updated)) {
			PluginDesc hairNodeDesc(hairNodeName, "Node");
			hairNodeDesc.add("geometry", hair_geom);
			hairNodeDesc.add("material", hair_mtl);
			hairNodeDesc.add("transform", AttrTransformFromBlTransform(ob.matrix_world()));
			hairNodeDesc.add("objectID", ob.pass_index());

			m_exporter->export_plugin(hairNodeDesc);
		}
	}
}