/*
 * Corebryo
 * Copyright (c) 2026 Jonathan Den Haerynck
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ObjLoader.h"

#include "../../../Math/MathTypes.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>

Mesh ObjLoader::LoadOBJ(
    const std::string& Path,
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkQueue Queue,
    uint32_t QueueFamily)
{
    /* Parse OBJ file and build GPU mesh buffers. */

    Mesh mesh{};

    std::ifstream file(Path);
    if (!file.is_open())
    {
        std::fprintf(stderr, "ObjLoader::LoadOBJ: Failed to open %s\n", Path.c_str());
        return mesh;
    }

    /* Raw vertex data */
    std::vector<Vec3> positions;
    std::vector<uint32_t> indices;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.size() < 2)
        {
            continue;
        }

        /* Vertex position */
        if (line[0] == 'v' && line[1] == ' ')
        {
            std::istringstream iss(line.substr(2));
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            if (iss >> x >> y >> z)
            {
                positions.emplace_back(x, y, z);
            }
            continue;
        }

        /* Triangle face */
        if (line[0] == 'f' && line[1] == ' ')
        {
            std::istringstream iss(line.substr(2));
            uint32_t faceIndices[3]{};
            int faceCount = 0;

            for (int i = 0; i < 3; ++i)
            {
                std::string token;
                if (!(iss >> token))
                {
                    break;
                }

                /* Strip texture and normal indices */
                size_t slashPos = token.find('/');
                if (slashPos != std::string::npos)
                {
                    token = token.substr(0, slashPos);
                }

                if (token.empty())
                {
                    continue;
                }

                int index = std::stoi(token);
                if (index > 0)
                {
                    faceIndices[faceCount++] = static_cast<uint32_t>(index - 1);
                }
            }

            if (faceCount == 3)
            {
                indices.push_back(faceIndices[0]);
                indices.push_back(faceIndices[1]);
                indices.push_back(faceIndices[2]);
            }
        }
    }

    if (positions.empty())
    {
        std::fprintf(stderr, "ObjLoader::LoadOBJ: No positions in %s\n", Path.c_str());
        return mesh;
    }

    /* Setup mesh counts */
    mesh.VertexCount = static_cast<uint32_t>(positions.size());
    mesh.HasIndex = !indices.empty();
    mesh.IndexCount = static_cast<uint32_t>(indices.size());

    /* Generate per-vertex normals */
    std::vector<Vec3> normals(positions.size(), Vec3(0.0f, 0.0f, 0.0f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        uint32_t ia = indices[i + 0];
        uint32_t ib = indices[i + 1];
        uint32_t ic = indices[i + 2];

        if (ia >= positions.size() || ib >= positions.size() || ic >= positions.size())
        {
            continue;
        }

        const Vec3& a = positions[ia];
        const Vec3& b = positions[ib];
        const Vec3& c = positions[ic];

        Vec3 ab = b - a;
        Vec3 ac = c - a;
        Vec3 n = Vec3::Cross(ab, ac);

        normals[ia] = normals[ia] + n;
        normals[ib] = normals[ib] + n;
        normals[ic] = normals[ic] + n;
    }

    for (Vec3& n : normals)
    {
        n = n.Normalized();
    }

    /* Interleave position and normal data */
    std::vector<float> vertexData;
    vertexData.reserve(positions.size() * 6);

    for (size_t i = 0; i < positions.size(); ++i)
    {
        const Vec3& p = positions[i];
        const Vec3& n = normals[i];

        vertexData.push_back(p.x);
        vertexData.push_back(p.y);
        vertexData.push_back(p.z);
        vertexData.push_back(n.x);
        vertexData.push_back(n.y);
        vertexData.push_back(n.z);
    }

    /* Upload vertex buffer */
    if (!mesh.VertexBuffer.CreateVertexBuffer(
        PhysicalDevice,
        Device,
        Queue,
        QueueFamily,
        vertexData.data(),
        vertexData.size() * sizeof(float)))
    {
        std::fprintf(stderr, "ObjLoader::LoadOBJ: Failed to create vertex buffer\n");
        mesh.Destroy(Device);
        return Mesh{};
    }

    /* Upload index buffer if present */
    if (mesh.HasIndex)
    {
        if (!mesh.IndexBuffer.CreateIndexBuffer(
            PhysicalDevice,
            Device,
            Queue,
            QueueFamily,
            indices.data(),
            mesh.IndexCount))
        {
            std::fprintf(stderr, "ObjLoader::LoadOBJ: Failed to create index buffer\n");
            mesh.Destroy(Device);
            return Mesh{};
        }
    }

    return mesh;
}
