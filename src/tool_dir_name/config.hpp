#pragma once

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <json/json.h>
#include <fstream>


#define HILL_SHADE_CONFIG_DEM_FILE        			"dem_file"
#define HILL_SHADE_CONFIG_IMA_OUT_PATH       		"image_out_path"
#define HILL_SHADE_CONFIG_B_LAYER     				"begin_layer"
#define HILL_SHADE_CONFIG_E_LAYER		        	"end_layer"
#define HILL_SHADE_CONFIG_I_WIDTH		        	"image_width"
#define HILL_SHADE_CONFIG_I_HEITH		        	"image_height"
#define HILL_SHADE_CONFIG_PYRAMID					"pyramid"
#define HILL_SHADE_CONFIG_RANDER					"rander"
#define HILL_SHADE_CONFIG_VALUE						"value"
#define HILL_SHADE_CONFIG_R							"r"
#define HILL_SHADE_CONFIG_G							"g"
#define HILL_SHADE_CONFIG_B							"b"
#define HILL_SHADE_CONFIG_TOP_ALPHA					"top_alpha"
#define HILL_SHADE_CONFIG_TIF_WIDTH					"tif_width"
#define HILL_SHADE_CONFIG_TIF_HEIGHT				"tif_height"
#define HILL_SHADE_CONFIG_PROJ						"proj_db_path"



struct pyramid_config
{
	int begin_layer;  //开始层
	int end_layer;   // 结束层
	int image_width;  // 生成图片宽度
	int image_height; // 生成图片高度
};
struct evel_color
{
	double value;
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

typedef std::vector<evel_color> rander_config;

class Config
{
public:
	// dem 文件路径
	std::string dem_file;
	float top_alpha;
	int tif_width;

	pyramid_config pyramid;
	rander_config rander;

	bool read_config(std::string config_path)
	{
		bool status = false;
		std::fstream input;
		input.open(config_path, std::ios::binary | std::ios::in);
		if (input.is_open())
		{
			Json::Reader reader;
			Json::Value root;

			if (reader.parse(input, root))
			{

				dem_file = root[HILL_SHADE_CONFIG_DEM_FILE].asString();
				Json::Value pyramidJson = root[HILL_SHADE_CONFIG_PYRAMID];
				pyramid.begin_layer = pyramidJson[HILL_SHADE_CONFIG_B_LAYER].asInt();
				pyramid.end_layer = pyramidJson[HILL_SHADE_CONFIG_E_LAYER].asInt();
				pyramid.image_width = pyramidJson[HILL_SHADE_CONFIG_I_WIDTH].asInt();
				pyramid.image_height = pyramidJson[HILL_SHADE_CONFIG_I_HEITH].asInt();

				top_alpha = root[HILL_SHADE_CONFIG_TOP_ALPHA].asFloat();
				tif_width = root[HILL_SHADE_CONFIG_TIF_WIDTH].asInt();

				Json::Value randerJson = root[HILL_SHADE_CONFIG_RANDER];
				for (Json::ValueIterator it = randerJson.begin(); it != randerJson.end(); ++it)
				{
					evel_color color;
					color.value = (*it)[HILL_SHADE_CONFIG_VALUE].asDouble();
					color.r = (*it)[HILL_SHADE_CONFIG_R].asUInt();
					color.g = (*it)[HILL_SHADE_CONFIG_G].asUInt();
					color.b = (*it)[HILL_SHADE_CONFIG_B].asUInt();
					rander.push_back(color);
				}
			}
			status = true;
		}

		input.close();
		return status;
	}
	

};

#endif //CONFIG_HPP
