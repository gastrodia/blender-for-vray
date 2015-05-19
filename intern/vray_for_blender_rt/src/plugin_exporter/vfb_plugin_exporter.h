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

#ifndef VRAY_FOR_BLENDER_PLUGIN_EXPORTER_H
#define VRAY_FOR_BLENDER_PLUGIN_EXPORTER_H

#include "cgr_config.h"
#include "vfb_util_defines.h"
#include "vfb_rna.h"
#include "vfb_plugin_attrs.h"

#include "BLI_math.h"
#include "MEM_guardedalloc.h"
#include "RNA_types.h"
#include "RNA_access.h"
#include "RNA_blender_cpp.h"

#include <vraysdk.hpp>
#include <map>
#include <set>

#include <boost/bind.hpp>
#include <boost/function.hpp>


namespace VRayForBlender {


enum ExpoterType {
	ExpoterTypeFile = 0,
	ExpoterTypeCloud,
	ExpoterTypeZMQ,
	ExpoterTypeAppSDK,
};


struct ExpoterCallback {
	typedef boost::function<void(void)> CallbackFunction;

	ExpoterCallback() {}
	ExpoterCallback(CallbackFunction _cb):
	    cb(_cb)
	{}

	operator bool () const {
		return !!(cb);
	}

	CallbackFunction cb;
};


struct RenderImage {
	RenderImage():
	    pixels(nullptr),
	    w(0),
	    h(0)
	{}

	virtual ~RenderImage() {}

	operator bool () const {
		return !!(pixels);
	}

	void free() {
		FreePtrArr(pixels);
	}

	float *pixels;
	int    w;
	int    h;
};


class PluginExporter
{
public:
	virtual             ~PluginExporter()=0;

	virtual void         init()=0;
	virtual void         free()=0;

	virtual void         sync()  {}
	virtual void         start() {}
	virtual void         stop()  {}

	virtual void         export_vrscene(const std::string &filepath) {}

	virtual AttrPlugin   export_plugin(const PluginDesc &pluginDesc)=0;
	virtual int          remove_plugin(const std::string &pluginName) { return 0; }

	virtual RenderImage  get_image() { return RenderImage(); }
	virtual void         set_render_size(const int&, const int&) {}

	virtual void         set_callback_on_image_ready(ExpoterCallback cb)      { callback_on_image_ready = cb; }
	virtual void         set_callback_on_rt_image_updated(ExpoterCallback cb) { callback_on_rt_image_updated = cb; }

protected:
	ExpoterCallback      callback_on_image_ready;
	ExpoterCallback      callback_on_rt_image_updated;

};

PluginExporter* ExporterCreate(ExpoterType type);
void            ExporterDelete(PluginExporter *exporter);

} // namespace VRayForBlender

#endif // VRAY_FOR_BLENDER_PLUGIN_EXPORTER_H