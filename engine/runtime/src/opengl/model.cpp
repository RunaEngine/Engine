#include "opengl/model.h"
#include "utils/system.h"
#include "utils/logs.h"
#include <iostream>


namespace runa::runtime::opengl
{
    Model::~Model()
    {
        if (deferDeinit)
            deinit();
    }

    bool Model::init(const std::filesystem::path& filepath)
    {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			filepath.string().c_str(),
			aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			utils::Logs::error(importer.GetErrorString());
			return false;
		}

		std::filesystem::path parentpath = filepath.parent_path();
		fparentpath = &parentpath;

		constructScene(scene->mRootNode, scene);

		utils::Logs::log("Model loaded successfully: %zu meshes", assets.size());

        return true;
    }

    void Model::deinit()
    {
        for (auto& resource : assets)
        {
            resource.mesh.deinit();
        }
        assets.clear();
    }

    void Model::defer(bool value)
    {
        deferDeinit = value;
    }

    void Model::draw(Shader& shader, Camera& camera)
    {
        // Go over all meshes and draw each one
        for (auto& asset : assets)
        {
            asset.mesh.draw(
                shader,
                camera,
                asset.matrix, asset.position, asset.rotation, asset.scale
            );
        }
    }

	void Model::constructScene(aiNode* node, const aiScene* scene)
	{
		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			Asset asset;
			std::vector<Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<Texture> textures;

			// Vertex
			vertices.reserve(mesh->mNumVertices);
			for (size_t index = 0; index < mesh->mNumVertices; index++) {
				Vertex vertex;
				aiVector3D position = mesh->mVertices[index];
				aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[index] : aiVector3D(0);
				aiColor4D color = mesh->HasVertexColors(0) ? mesh->mColors[0][index] : aiColor4D(1);
				aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][index] : aiVector3D(0);
				
				vertex.position = glm::vec3(position.x, position.y, position.z);
				vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
				vertex.color = glm::vec3(color.r, color.g, color.b);
				vertex.texUV = glm::vec2(texCoord.x, texCoord.y);

				vertices.push_back(vertex);
			}

			// Indices
			for (size_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) 
			{
				aiFace face = mesh->mFaces[faceIndex];
				if (face.mNumIndices > 0)
					indices.reserve(indices.size() + face.mNumIndices);
				for (size_t indIndex = 0; indIndex < face.mNumIndices; indIndex++)
				{
					GLuint indice = face.mIndices[indIndex];
					indices.push_back(indice);
				}
			}

			// Texture
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			Texture diffuse;
			aiString texturePath;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
				std::string textureFileName = texturePath.C_Str();
				utils::Logs::log("%s", textureFileName.c_str());
				// Check if the texture is embedded or external
				if (textureFileName.c_str()[0] == '*') {
					// Embedded texture
					unsigned int textureIndex = std::stoul(textureFileName.substr(1));
					aiTexture* embeddedTexture = scene->mTextures[textureIndex];
					std::vector<uint8_t> buffer;
					if (embeddedTexture->mHeight == 0) {
						uint8_t* data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
						buffer.assign(data, data + embeddedTexture->mWidth);
					}
					else {
						size_t size = embeddedTexture->mWidth * embeddedTexture->mHeight * sizeof(aiTexel);
						uint8_t* data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
						buffer.assign(data, data + size);
					}
					if (diffuse.init(buffer, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE))
					{
						diffuse.defer(false);
					}
				} else {
					// External texture
					std::filesystem::path ftexturePath = *fparentpath;
					ftexturePath.append(texturePath.C_Str());
					if (diffuse.init(ftexturePath, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE))
					{
						diffuse.defer(false);
					}
				}
			}
			if (diffuse.getID() != 0) textures.push_back(diffuse);

			Texture specular;
			if (material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS) {
				std::string textureFileName = texturePath.C_Str();
				// Check if the texture is embedded or external
				if (textureFileName.c_str()[0] == '*') {
					// Embedded texture
					unsigned int textureIndex = std::stoul(textureFileName.substr(1));
					aiTexture* embeddedTexture = scene->mTextures[textureIndex];
					std::vector<uint8_t> buffer;
					if (embeddedTexture->mHeight == 0) {
						uint8_t* data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
						buffer.assign(data, data + embeddedTexture->mWidth);
					}
					else {
						size_t size = embeddedTexture->mWidth * embeddedTexture->mHeight * sizeof(aiTexel);
						uint8_t* data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
						buffer.assign(data, data + size);
					}
					if (specular.init(buffer, "specular", textures.size(), 0, GL_UNSIGNED_BYTE))
					{
						specular.defer(false);
					}
				} else {
					// External texture
					std::filesystem::path ftexturePath = *fparentpath;
					ftexturePath.append(texturePath.C_Str());
					if (specular.init(ftexturePath, "specular", textures.size(), 0, GL_UNSIGNED_BYTE))
					{
						specular.defer(false);
					}
				}
			}
			if (specular.getID() != 0) textures.push_back(specular);

			// Mesh
			if (asset.mesh.init(vertices, indices, textures))
			{
				glm::mat4 nodeMatrix = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
				asset.matrix = nodeMatrix;
				asset.position = glm::vec3(nodeMatrix[3]);
				asset.scale = glm::vec3(
					glm::length(glm::vec3(nodeMatrix[0])),
					glm::length(glm::vec3(nodeMatrix[1])),
					glm::length(glm::vec3(nodeMatrix[2]))
				);
				// Normalize the rotation matrix by removing scale
				glm::mat3 rotationMatrix = glm::mat3(nodeMatrix);
				rotationMatrix[0] = glm::normalize(rotationMatrix[0]);
				rotationMatrix[1] = glm::normalize(rotationMatrix[1]);
				rotationMatrix[2] = glm::normalize(rotationMatrix[2]);
				asset.rotation = glm::quat_cast(rotationMatrix);
				asset.mesh.defer(false);
				assets.push_back(asset);
			}
		}

		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			aiNode* children = node->mChildren[i];
			if (!children) continue;
			constructScene(children, scene);
		}
	}

	/*
    std::vector<uint8_t> Model::getData(const std::filesystem::path& parentpath)
    {
        std::vector<uint8_t> gltfData;

        // Get data from buffer or .bin file
        if (!resource->get().buffers.empty())
        {
            fastgltf::Buffer& buffer = resource->get().buffers[0];
            std::visit([&](auto&& source) -> void {
                using T = std::decay_t<decltype(source)>;

                if constexpr (std::is_same_v<T, fastgltf::sources::Vector>) {
                    std::vector<std::byte> bytes = source.bytes;
                    gltfData.reserve(bytes.size());
                    std::transform(source.bytes.begin(), source.bytes.end(), std::back_inserter(gltfData),
                        [](std::byte b) { return static_cast<uint8_t>(b); });
                }
                else if constexpr (std::is_same_v<T, fastgltf::sources::URI>) {
                    fastgltf::URI uri = source.uri;
                    std::vector<uint8_t> data;
                    if (utils::readFile(parentpath / uri.fspath(), data))
                    {
                        gltfData = data;
                    }
                }
            }, buffer.data);
        }

        return std::move(gltfData);
    }

    void Model::traverseNode(unsigned int nextNode, glm::mat4 matrix)
    {
    	if (resource->get().nodes.size() < nextNode)
    		return;

        // Current node
		auto& node = resource->get().nodes[nextNode];

		// Get translation if it exists
		glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
		// Get quaternion if it exists
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		// Get scale if it exists
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
		// Get matrix if it exists
		glm::mat4 matNode = glm::mat4(1.0f);

    	std::visit([&](auto&& arg) -> void {
			using T = std::decay_t<decltype(arg)>;

    		if constexpr (std::is_same_v<T, fastgltf::TRS>) {
				// TRS
    			fastgltf::TRS transform = arg;
				translation = glm::vec3(transform.translation.x(), transform.translation.y(), transform.translation.z());
				rotation = glm::quat(transform.rotation.w(), transform.rotation.x(), transform.rotation.y(), transform.rotation.z());
				scale = glm::vec3(transform.scale.x(), transform.scale.y(), transform.scale.z());
			}
			if constexpr (std::is_same_v<T, fastgltf::math::fmat4x4>) {
				// Matrix
				fastgltf::math::fmat4x4 fmat4 = arg;
				matNode = glm::mat4(
					fmat4[0][0],  fmat4[0][1],  fmat4[0][2],  fmat4[0][3],
					fmat4[1][0],  fmat4[1][1],  fmat4[1][2],  fmat4[1][3],
					fmat4[2][0],  fmat4[2][1],  fmat4[2][2],  fmat4[2][3],
					fmat4[3][0],  fmat4[3][1],  fmat4[3][2],  fmat4[3][3]
				);
			}
		}, node.transform);

		// Initialize matrices
		glm::mat4 trans = glm::mat4(1.0f);
		glm::mat4 rot = glm::mat4(1.0f);
		glm::mat4 sca = glm::mat4(1.0f);

		// Use translation, rotation, and scale to change the initialized matrices
		trans = glm::translate(trans, translation);
		rot = glm::mat4_cast(rotation);
		sca = glm::scale(sca, scale);

		// Multiply all matrices together
		glm::mat4 matNextNode = matrix * matNode * trans * rot * sca;

		// Check if the node contains a mesh and if it does load it
    	if (node.meshIndex.has_value())
    	{
			Asset resource;
    		resource.position = translation;
    		resource.rotation = rotation;
    		resource.scale = scale;
    		resource.matrix = matNextNode;
    		resource.mesh = loadMesh(node.meshIndex.value());
    		scene.push_back(std::move(resource));
    	}

		// Check if the node has children, and if it does, apply this function to them with the matNextNode
    	if (!node.children.empty())
    	{
    		for (size_t children : node.children)
    			traverseNode(children, matNextNode);
    	}
    }

    Mesh Model::loadMesh(unsigned int indice)
    {
    	Mesh mesh;
    	if (resource->get().meshes.empty())
    		return mesh;

    	// Get all accessor indices
    	size_t posIndex = -1;
    	size_t normalIndex = -1;
    	size_t texIndex = -1;
    	size_t indIndex = -1;
    	auto& attributes = resource->get().meshes[indice].primitives[0].attributes;
    	for (auto& attr : attributes)
    	{
    		if (attr.name == "POSITION") posIndex = attr.accessorIndex;
    		else if (attr.name == "NORMAL") normalIndex = attr.accessorIndex;
    		else if (attr.name == "TEXCOORD_0") texIndex = attr.accessorIndex;
    	}

    	if (!resource->get().meshes[indice].primitives[0].indicesAccessor.has_value())
    		return mesh;

    	indIndex = resource->get().meshes[indice].primitives[0].indicesAccessor.value();

    	if (posIndex == -1 || normalIndex == -1 || texIndex == -1)
    		return mesh;

    	// Use accessor indices to get all vertices components
    	std::vector<float> posVec = getFloats(resource->get().accessors[posIndex]);
    	std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
    	std::vector<float> normalVec = getFloats(resource->get().accessors[normalIndex]);
    	std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
    	std::vector<float> texVec = getFloats(resource->get().accessors[texIndex]);
    	std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

    	// Combine all the vertex components and also get the indices and textures
    	std::vector<Vertex> vertices = assembleVertices(positions, normals, texUVs);
    	std::vector<GLuint> indices = getIndices(resource->get().accessors[indIndex]);
    	std::vector<Texture> textures = getTextures();

    	// Combine the vertices, indices, and textures into a mesh
    	mesh.init(vertices, indices, textures);
    	mesh.defer(false);
    	return mesh;
    }

    std::vector<float> Model::getFloats(fastgltf::Accessor& accessor)
    {
    	std::vector<float> vec;

		if (!accessor.bufferViewIndex.has_value())
			return vec;

		size_t buffViewInd = accessor.bufferViewIndex.value();
		size_t count = accessor.count;
		size_t accByteOffset = accessor.byteOffset;
		fastgltf::AccessorType type = accessor.type;

		fastgltf::BufferView& bufferView = resource->get().bufferViews[buffViewInd];
		size_t byteOffset = bufferView.byteOffset;

		// Initialize numPerVert with error handling
		unsigned int numPerVert = 0;
		switch (type)
		{
			case fastgltf::AccessorType::Scalar: numPerVert = 1; break;
			case fastgltf::AccessorType::Vec2: numPerVert = 2; break;
			case fastgltf::AccessorType::Vec3: numPerVert = 3; break;
			case fastgltf::AccessorType::Vec4: numPerVert = 4; break;
			default: 
				// Handle error: unknown accessor type
				return vec;
		}

		size_t beginningOfData = byteOffset + accByteOffset;
		size_t lengthOfData = count * 4 * numPerVert;

		// Bounds check
		if (beginningOfData + lengthOfData > bin->size())
		{
			// Handle error: buffer overflow
			return vec;
		}

		vec.reserve(count * numPerVert);  // Optimize allocations

		for (size_t i = beginningOfData; i < beginningOfData + lengthOfData; i += 4)
		{
			float value;
			memcpy(&value, &(*bin)[i], sizeof(float));
			vec.push_back(value);
		}

		return vec;
    }

    std::vector<Texture> Model::getTextures()
    {
    	std::map<const char*, Texture> textures;

		for (auto& imageData : resource->get().images)
		{
			std::vector<uint8_t> textData;
			std::string name = imageData.name;

			std::visit([&](auto&& source) -> void {
				using T = std::decay_t<decltype(source)>;

				if constexpr (std::is_same_v<T, fastgltf::sources::Vector>) {
					std::vector<std::byte> bytes = source.bytes;
					textData.reserve(bytes.size());
					std::transform(source.bytes.begin(), source.bytes.end(), std::back_inserter(textData),
						[](std::byte b) { return static_cast<uint8_t>(b); });
				}
				else if constexpr (std::is_same_v<T, fastgltf::sources::URI>) {
					fastgltf::URI uri = source.uri;
					std::vector<uint8_t> fdata;
					if (utils::readFile(*fparentpath / uri.fspath(), fdata))
					{
						textData = fdata;
						name = uri.fspath().filename().string();
					}
				}
			}, imageData.data);

			// Load diffuse texture
			if (name.find("baseColor") != std::string::npos ||
				name.find("Color") != std::string::npos ||
				name.find("Diffuse") != std::string::npos)
			{
				Texture diffuse;
				if (diffuse.init(textData, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE)) {
					diffuse.defer(false);
					textures.insert_or_assign(name.c_str(), diffuse);
				}
			}
			// Load specular texture
			else if (name.find("metallicRoughness") != std::string::npos ||
					 name.find("Roughness") != std::string::npos ||
					 name.find("Specular") != std::string::npos)
			{
				Texture specular;
				if (specular.init(textData, "specular", textures.size(), 0, GL_UNSIGNED_BYTE)) {
					specular.defer(false);
					textures.insert_or_assign(name.c_str(), specular);
				}
			}
		}

    	std::vector<Texture> texturesVec;
    	texturesVec.reserve(textures.size());
    	for (const auto& [key, texture] : textures) {
    		texturesVec.push_back(texture);
    	}

    	return texturesVec;
    }

    std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> floatVec)
    {
    	const unsigned int floatsPerVector = 3;

    	std::vector<glm::vec3> vectors;
    	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
    	{
    		vectors.emplace_back(0, 0, 0);

    		for (unsigned int j = 0; j < floatsPerVector; j++)
    		{
    			vectors.back()[j] = floatVec[i + j];
    		}
    	}
    	return vectors;
    }

    std::vector<glm::vec2> Model::groupFloatsVec2(std::vector<float> floatVec)
    {
    	const unsigned int floatsPerVector = 2;

    	std::vector<glm::vec2> vectors;
    	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
    	{
    		vectors.push_back(glm::vec2(0, 0));

    		for (unsigned int j = 0; j < floatsPerVector; j++)
    		{
    			vectors.back()[j] = floatVec[i + j];
    		}
    	}
    	return vectors;
    }

    std::vector<GLuint> Model::getIndices(fastgltf::Accessor& accessor)
    {
    	std::vector<GLuint> indices;

    	// Get properties from the accessor
    	size_t buffViewInd = accessor.bufferViewIndex.value();
    	size_t count = accessor.count;
    	size_t accByteOffset = accessor.byteOffset;
    	fastgltf::ComponentType componentType = accessor.componentType;

    	// Get properties from the bufferView
    	fastgltf::BufferView& bufferView = resource->get().bufferViews[buffViewInd];
    	size_t byteOffset = bufferView.byteOffset;

    	// Get indices with regards to their type: unsigned int, unsigned short, or short
    	unsigned int beginningOfData = byteOffset + accByteOffset;
    	size_t endOfData = beginningOfData + count * sizeof(unsigned int);

    	if (componentType == fastgltf::ComponentType::UnsignedInt)
    	{
    		for (size_t i = beginningOfData; i < endOfData; i += 4)
    		{
    	        unsigned int value = *reinterpret_cast<const unsigned int*>(&(*bin)[i]);
    	        indices.push_back(static_cast<GLuint>(value));
    		}
    	}
    	else if (componentType == fastgltf::ComponentType::UnsignedShort)
    	{
			size_t requiredSize = byteOffset + accByteOffset + count * 2;
			if (requiredSize > bin->size())
			{
				// Handle error appropriately
				return indices; // or throw exception
			}

    		for (unsigned int i = beginningOfData; i < requiredSize; i += 2)
    		{
    			unsigned char bytes[] = { (*bin)[i], (*bin)[i + 1] };
    			unsigned short value;
    			memcpy(&value, bytes, sizeof(unsigned short));
    			indices.push_back((GLuint)value);
    		}
    	}
    	else if (componentType == fastgltf::ComponentType::Short)
    	{
    		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
    		{
    			unsigned char bytes[] = { bin->at(i), bin->at(i + 1) };
    			short value;
    			memcpy(&value, bytes, sizeof(short));
    			indices.push_back((GLuint)value);
    		}
    	}

    	return indices;
    }

    std::vector<Vertex> Model::assembleVertices(std::vector<glm::vec3> positions,
                                                std::vector<glm::vec3> normals, std::vector<glm::vec2> texCoords)
    {
    	std::vector<Vertex> vertices;
    	for (int i = 0; i < positions.size(); i++)
    	{
    		vertices.push_back
			(
				Vertex
				{
					positions[i],
					normals[i],
					glm::vec3(1.0f, 1.0f, 1.0f),
					texCoords[i]
				}
			);
    	}
    	return vertices;
    }
	*/
}
