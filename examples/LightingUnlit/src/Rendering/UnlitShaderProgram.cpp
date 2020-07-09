/*
 * Copyright (c) 2002 - present, H. Hernan Saez
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "UnlitShaderProgram.hpp"

using namespace crimild;

UnlitShaderProgram::UnlitShaderProgram( void ) noexcept
{
    auto createShader = []( Shader::Stage stage, std::string path ) {
        return crimild::alloc< Shader >(
            stage,
            FileSystem::getInstance().readFile(
                FilePath {
                    .path = path,
                }.getAbsolutePath()
            )
        );
    };

    setShaders(
        Array< SharedPointer< Shader >> {
            createShader(
                Shader::Stage::VERTEX,
                "assets/shaders/unlit.vert.spv"
            ),
            createShader(
                Shader::Stage::FRAGMENT,
                "assets/shaders/unlit.frag.spv"
            ),
        }
    );

    vertexLayouts = { VertexP3N3TC2::getLayout() };

    descriptorSetLayouts = {
        [] {
            auto layout = crimild::alloc< DescriptorSetLayout >();
            layout->bindings = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .stage = Shader::Stage::VERTEX,
                },
            };
            return layout;
        }(),
        [] {
            auto layout = crimild::alloc< DescriptorSetLayout >();
            layout->bindings = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .stage = Shader::Stage::FRAGMENT,
                },
                {
                    .descriptorType = DescriptorType::TEXTURE,
                    .stage = Shader::Stage::FRAGMENT,
                },
            };
            return layout;
        }(),
        [] {
            auto layout = crimild::alloc< DescriptorSetLayout >();
            layout->bindings = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .stage = Shader::Stage::VERTEX,
                },
            };
            return layout;
        }(),
    };
}
