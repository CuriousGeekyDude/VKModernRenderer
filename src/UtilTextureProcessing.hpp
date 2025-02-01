#pragma once

#include "AllInitialValues.hpp"
#include "Material.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <execution>
#include <filesystem>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "stb_image.h"
#include "stb_image_resize.h"

namespace UtilTextureProcessing
{
	namespace fs = std::filesystem;

	std::string replaceAll(const std::string& str, const std::string& oldSubStr, const std::string& newSubStr)
	{
		std::string result = str;

		for (size_t p = result.find(oldSubStr); p != std::string::npos; p = result.find(oldSubStr))
			result.replace(p, oldSubStr.length(), newSubStr);

		return result;
	}

	/* Convert 8-bit ASCII string to upper case */
	std::string lowercaseString(const std::string& s)
	{
		std::string out(s.length(), ' ');
		std::transform(s.begin(), s.end(), out.begin(), tolower);
		return out;
	}

	/* find a file in directory which "almost" coincides with the origFile (their lowercase versions coincide) */
	std::string findSubstitute(const std::string& origFile)
	{
		// Make absolute path
		auto apath = fs::absolute(fs::path(origFile));
		// Absolute lowercase filename [we compare with it]
		auto afile = lowercaseString(apath.filename().string());
		// Directory where in which the file should be
		auto dir = fs::path(origFile).remove_filename();

		// Iterate each file non-recursively and compare lowercase absolute path with 'afile'
		for (auto& p : fs::directory_iterator(dir))
			if (afile == lowercaseString(p.path().filename().string()))
				return p.path().string();
		

		fflush(stdout);

		return std::string{};
	}

	std::string fixTextureFile(const std::string& file)
	{
		if (true == fs::exists(file)) {
			return file;
		}
		else {
			auto lv_subsituteString = findSubstitute(file);
			return lv_subsituteString;
		}
		 
	}


	std::string convertTexture(const std::string& file, const std::string& basePath, 
		std::unordered_map<std::string, uint32_t>& opacityMapIndices, 
		const std::vector<std::string>& opacityMaps)
	{
		const int maxNewWidth = 512;
		const int maxNewHeight = 512;

		auto srcFile = replaceAll(basePath + file, "\\", "/");

		

		auto newFile = std::string(VulkanEngine::InitialValues::lv_texturefilesPath) + lowercaseString(replaceAll(replaceAll(srcFile, "..", "__"), "/", "__") + std::string("__rescaled")) + std::string(".png");



		// load this image
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(fixTextureFile(srcFile).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		uint8_t* src = pixels;
		texChannels = STBI_rgb_alpha;

		std::vector<uint8_t> tmpImage(maxNewWidth * maxNewHeight * 4);

		if (!src)
		{
			const char* lv_failureReason = stbi_failure_reason();
			printf("Failed to load [%s] texture\nReason: [%s]\n", srcFile.c_str(), lv_failureReason);
			texWidth = maxNewWidth;
			texHeight = maxNewHeight;
			texChannels = STBI_rgb_alpha;
			src = tmpImage.data();
		}
		else
		{
			printf("Loaded [%s] %dx%d texture with %d channels\n", srcFile.c_str(), texWidth, texHeight, texChannels);
		}

		if (opacityMapIndices.count(file) > 0)
		{
			const auto opacityMapFile = replaceAll(basePath + opacityMaps[opacityMapIndices[file]], "\\", "/");
			int opacityWidth, opacityHeight;
			stbi_uc* opacityPixels = stbi_load(fixTextureFile(opacityMapFile).c_str(), &opacityWidth, &opacityHeight, nullptr, 1);

			if (!opacityPixels)
			{
				const char* lv_failureReason = stbi_failure_reason();
				printf("Failed to load opacity mask [%s]\nReason: [%s]\n", opacityMapFile.c_str(), lv_failureReason);
			}

			assert(opacityPixels);
			assert(texWidth == opacityWidth);
			assert(texHeight == opacityHeight);

			// store the opacity mask in the alpha component of this image
			if (opacityPixels)
				for (int y = 0; y != opacityHeight; y++)
					for (int x = 0; x != opacityWidth; x++)
						src[(y * opacityWidth + x) * texChannels + 3] = opacityPixels[y * opacityWidth + x];

			stbi_image_free(opacityPixels);
		}

		const uint32_t imgSize = texWidth * texHeight * texChannels;
		std::vector<uint8_t> mipData(imgSize);
		uint8_t* dst = mipData.data();

		const int newW = std::min(texWidth, maxNewWidth);
		const int newH = std::min(texHeight, maxNewHeight);

		stbir_resize_uint8(src, texWidth, texHeight, 0, dst, newW, newH, 0, texChannels);

		auto lv_returnstbiWritePng = stbi_write_png(newFile.c_str(), newW, newH, texChannels, dst, 0);

		if (pixels)
			stbi_image_free(pixels);

		return newFile;
	}


	void convertAndDownscaleAllTextures(
		const std::vector<SceneConverter::Material>& l_materials, 
		const std::string& l_basePath, std::vector<std::string>& l_files, 
		std::vector<std::string>& l_opacityMaps)
	{
		using namespace SceneConverter;

		std::unordered_map<std::string, uint32_t> opacityMapIndices(l_files.size());

		for (const auto& m : l_materials) {
			if (m.m_opacityMap != INVALID_TEXTURE && m.m_albedoMap != INVALID_TEXTURE) {
				opacityMapIndices[l_files[m.m_albedoMap]] = (uint32_t)m.m_opacityMap;
			}
		}



		auto converter = [&](const std::string& s) -> std::string
			{
				return convertTexture(s, l_basePath, opacityMapIndices, l_opacityMaps);
			};

		std::transform(std::execution::par, std::begin(l_files), std::end(l_files), std::begin(l_files), 
			converter);
	}
}