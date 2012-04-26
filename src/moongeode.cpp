
// Copyright (c) 2011-2012, Daniel M�ller <dm@g4t3.de>
// Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the Computer Graphics Systems Group at the 
//     Hasso-Plattner-Institute (HPI), Germany nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#include "moongeode.h"

#include "himmel.h"
#include "himmelquad.h"
#include "abstractastronomy.h"
#include "mathmacros.h"

#include "shaderfragment/common.h"

#include <osg/Geometry>
#include <osg/TextureCubeMap>
#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <assert.h>


namespace osgHimmel
{

// Transforms a quad to the moons position into the canopy. Then 
// generates a circle with sphere normals (and normals from cube map)
// representing a perfect sphere in space.
// Applies lighting from sun - moon phase is always correct and no 
// separate calculation is required. Correct Moon rotation is currently
// faked (face towards earth is incorrect due to missing librations etc).

MoonGeode::MoonGeode(const char* cubeMapFilePath)
:   osg::Geode()

,   m_program(new osg::Program)
,   m_vShader(new osg::Shader(osg::Shader::VERTEX))
,   m_fShader(new osg::Shader(osg::Shader::FRAGMENT))

,   m_hquad(new HimmelQuad())

,   u_moon(NULL)
,   u_moonr(NULL)
,   u_moonCube(NULL)
,   u_R(NULL)
,   u_sunShine(NULL)    // [0,1,2] = color; [3] = intensity
,   u_earthShine(NULL)  // [0,1,2] = color;

,   m_earthShineColor(defaultEarthShineColor())
,   m_earthShineScale(defaultEarthShineIntensity())
{
    setName("Moon");

    m_scale = defaultScale();

    osg::StateSet* stateSet = getOrCreateStateSet();

    setupNode(stateSet);
    setupUniforms(stateSet);
    setupShader(stateSet);
    setupTextures(stateSet, cubeMapFilePath);

    addDrawable(m_hquad);
};


MoonGeode::~MoonGeode()
{
};


void MoonGeode::update(const Himmel &himmel)
{
    const osg::Vec3 moonv = himmel.astro()->getMoonPosition(false);
    u_moon->set(osg::Vec4(moonv, himmel.astro()->getAngularMoonRadius() * m_scale));

    const osg::Vec3 moonrv = himmel.astro()->getMoonPosition(true);
    u_moonr->set(osg::Vec4(moonrv, moonv[3]));

    u_R->set(himmel.astro()->getMoonOrientation());

    u_earthShine->set(m_earthShineColor 
        * himmel.astro()->getEarthShineIntensity() * m_earthShineScale);
}


void MoonGeode::addUniformsToVariousStateSate(osg::StateSet* stateSet)
{
    if(!stateSet)
        return;

    assert(u_moon);
    stateSet->addUniform(u_moon);
    assert(u_moonr);
    stateSet->addUniform(u_moonr);
    assert(u_sunShine);
    stateSet->addUniform(u_sunShine);
    assert(u_earthShine);
    stateSet->addUniform(u_earthShine); 
}


void MoonGeode::setupUniforms(osg::StateSet* stateSet)
{
    u_moon = new osg::Uniform("moon", osg::Vec4(0.0, 0.0, 1.0, 1.0)); // [3] = apparent angular radius (not diameter!)
    stateSet->addUniform(u_moon);

    u_moonr = new osg::Uniform("moonr", osg::Vec4(0.0, 0.0, 1.0, 1.0)); // [3] = apparent angular radius (not diameter!)
    stateSet->addUniform(u_moonr);

    u_moonCube = new osg::Uniform("moonCube", 0);
    stateSet->addUniform(u_moonCube);

    u_R = new osg::Uniform("R", osg::Matrixd());
    stateSet->addUniform(u_R);

    u_sunShine = new osg::Uniform("sunShine"
        , osg::Vec4(defaultSunShineColor(), defaultSunShineIntensity()));
    stateSet->addUniform(u_sunShine);

    u_earthShine = new osg::Uniform("earthShine"
        , osg::Vec3(osg::Vec3(0, 0, 0)));
    stateSet->addUniform(u_earthShine);
}


void MoonGeode::setupNode(osg::StateSet* )
{
}


void MoonGeode::setupShader(osg::StateSet* stateSet)
{
    m_vShader->setShaderSource(getVertexShaderSource());
    m_fShader->setShaderSource(getFragmentShaderSource());

    m_program->addShader(m_vShader);
    m_program->addShader(m_fShader);

    stateSet->setAttributeAndModes(m_program, osg::StateAttribute::ON);
}


void MoonGeode::setupTextures(
    osg::StateSet* stateSet
,   const char* cubeMapFilePath)
{
    osg::ref_ptr<osg::TextureCubeMap> tcm(new osg::TextureCubeMap);

    tcm->setUnRefImageDataAfterApply(true);

    tcm->setInternalFormat(GL_RGBA);

    tcm->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tcm->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tcm->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

    tcm->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    tcm->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    std::string px = cubeMapFilePath; px.replace(px.find("?"), 1, "_px");
    std::string nx = cubeMapFilePath; nx.replace(nx.find("?"), 1, "_nx");
    std::string py = cubeMapFilePath; py.replace(py.find("?"), 1, "_py");
    std::string ny = cubeMapFilePath; ny.replace(ny.find("?"), 1, "_ny");
    std::string pz = cubeMapFilePath; pz.replace(pz.find("?"), 1, "_pz");
    std::string nz = cubeMapFilePath; nz.replace(nz.find("?"), 1, "_nz");

    tcm->setImage(osg::TextureCubeMap::POSITIVE_X, osgDB::readImageFile(px));
    tcm->setImage(osg::TextureCubeMap::NEGATIVE_X, osgDB::readImageFile(nx));
    tcm->setImage(osg::TextureCubeMap::POSITIVE_Y, osgDB::readImageFile(py));
    tcm->setImage(osg::TextureCubeMap::NEGATIVE_Y, osgDB::readImageFile(ny));
    tcm->setImage(osg::TextureCubeMap::POSITIVE_Z, osgDB::readImageFile(pz));
    tcm->setImage(osg::TextureCubeMap::NEGATIVE_Z, osgDB::readImageFile(nz));

    stateSet->setTextureAttributeAndModes(0, tcm, osg::StateAttribute::ON);

    u_moonCube->set(0);

    
    // generate lunar eclipse 1d-texture


}



const float MoonGeode::setScale(const float scale)
{
    osg::Vec4 temp;
    u_moon->get(temp);

    temp[3] = temp[3] / m_scale * scale;
    u_moon->set(temp);

    m_scale = scale;

    return getScale();
}

const float MoonGeode::getScale() const
{
    return m_scale;
}

const float MoonGeode::defaultScale()
{
    return 2.f;
}


const osg::Vec3 MoonGeode::setSunShineColor(const osg::Vec3 &color)
{
    osg::Vec4 sunShine;
    u_sunShine->get(sunShine);

    sunShine[0] = color[0];
    sunShine[1] = color[1];
    sunShine[2] = color[2];

    u_sunShine->set(sunShine);

    return getSunShineColor();
}

const osg::Vec3 MoonGeode::getSunShineColor() const
{
    osg::Vec4 sunShine;
    u_sunShine->get(sunShine);

    return osg::Vec3(sunShine[0], sunShine[1], sunShine[2]);
}

const osg::Vec3 MoonGeode::defaultSunShineColor()
{
    return osg::Vec3(1.0, 0.96, 0.80);
}


const float MoonGeode::setSunShineIntensity(const float intensity)
{
    osg::Vec4 sunShine;
    u_sunShine->get(sunShine);

    sunShine[3] = intensity;
    u_sunShine->set(sunShine);

    return getSunShineIntensity();
}

const float MoonGeode::getSunShineIntensity() const
{
    osg::Vec4 sunShine;
    u_sunShine->get(sunShine);

    return sunShine[3];
}

const float MoonGeode::defaultSunShineIntensity()
{
    return 24.f;
}


const osg::Vec3 MoonGeode::setEarthShineColor(const osg::Vec3 &color)
{
    m_earthShineColor = color;
    return m_earthShineColor;
}

const osg::Vec3 MoonGeode::getEarthShineColor() const
{
    return m_earthShineColor;
}

const osg::Vec3 MoonGeode::defaultEarthShineColor()
{
    return osg::Vec3(0.92, 0.96, 1.00);
}


const float MoonGeode::setEarthShineIntensity(const float intensity)
{
    m_earthShineScale = intensity;
    return m_earthShineScale;
}

const float MoonGeode::getEarthShineIntensity() const
{
    return m_earthShineScale;
}

const float MoonGeode::defaultEarthShineIntensity()
{
    return 1.f;
}




const std::string MoonGeode::getVertexShaderSource()
{
    return glsl_version_150() +

    +   PRAGMA_ONCE(main,

        // moon.xyz is expected to be normalized and moon.a the moons
        // angular diameter in rad.
        "uniform vec4 moon;\n"
        "uniform vec4 moonr;\n"
        "\n"
        "out mat4 m_tangent;\n"
        "out vec3 m_eye;\n"
        "\n"
        "const float SQRT2 = 1.41421356237;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec3 m = moonr.xyz;\n"
        "\n"
        //  tangent space of the unitsphere at m.
        "    vec3 u = normalize(cross(vec3(0, 0, 1), m));\n"
        "    vec3 v = normalize(cross(m, u));\n"
        "    m_tangent = mat4(vec4(u, 0.0), vec4(v, 0.0), vec4(m, 0.0), vec4(vec3(0.0), 1.0));\n"
        "\n"
        "    float mScale = tan(moon.a) * SQRT2;\n"
        "\n"
        "    m_eye = m - normalize(gl_Vertex.x * u + gl_Vertex.y * v) * mScale;\n"
        "\n"
        "    gl_TexCoord[0] = gl_Vertex;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * vec4(m_eye, 1.0);\n"
        "}");
}


const std::string MoonGeode::getFragmentShaderSource()
{
    return glsl_version_150()
    
    +   glsl_cmn_uniform()
    +   glsl_horizon()

    +   PRAGMA_ONCE(main,

        "uniform vec3 sun;\n"
        "\n"
        // moon.xyz is expected to be normalized and moon.a the moons
        // angular diameter in rad.
        "uniform vec4 moon;\n" 
        "\n"
        "uniform samplerCube moonCube;\n"
        "\n"
        "uniform mat4 R;\n"
        "\n"
        "uniform vec4 sunShine;\n"   // rgb as color and w as intensity.
        "uniform vec3 earthShine;\n" // rgb as color with premultiplied intensity and scale.
        "\n"
        "const float radius = 0.98;\n"
        "\n"
        "in vec3 m_eye;\n"
        "in mat4 m_tangent;\n"
        "\n"
        "const float PI               = 3.1415926535897932;\n"
        "const float TWO_OVER_THREEPI = 0.2122065907891938;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    float x = gl_TexCoord[0].x;\n"
        "    float y = gl_TexCoord[0].y;\n"
        "\n"
        "    float zz = radius * radius - x * x - y * y;\n"
        "    if(zz < 1.0 - radius)\n"
        "        discard;\n"
        "\n"
        "    vec3 eye = normalize(m_eye.xyz);\n"
        "\n"
        "    if(belowHorizon(eye))\n"
        "        discard;\n"
        "\n"
        "    float z = sqrt(zz);\n"
        "\n"
        // Moon Tanget Space
        "    vec3 mn = (m_tangent * vec4(x, y, z, 1.0)).xyz;\n"
        //"    vec3 mt = mn.zyx;\n"
        //"    vec3 mb = mn.xzy;\n"
        "\n"
//        // Texture Lookup direction -> "FrontFacing".
//        "    vec3 q = (vec4(mn.x, mn.y, mn.z, 1.0) * R).xyz;\n"
//        "\n"
//        "    vec4 c  = textureCube(moonCube, vec3(-q.x, q.y, -q.z));\n"
//        "    vec3 cn = (c.xyz) * 2.0 - 1.0;\n"
//        "    vec3 n = vec3(dot(cn, mt), dot(cn, mb), dot(cn, mn));\n"
//        "\n"
        "    vec3 m = moon.xyz;\n"
        "\n"
        // Hapke-Lommel-Seeliger approximation of the moons reflectance function.

        "    float cos_p = clamp(dot(eye, sun), 0.0, 1.0);\n"
        "    float p     = acos(cos_p);\n"
        "    float tan_p = tan(p);\n"
        "\n"
        "    float dot_ne = dot(mn, eye);\n"
        "    float dot_nl = dot(mn, sun);\n"
        "\n"
        "    float g = 0.6;\n" // surface densitiy parameter which determines the sharpness of the peak at the full Moon
        "    float t = 0.1;\n" // small amount of forward scattering
        "\n"
        // Retrodirective.
        "    float _R = 2.0 - tan_p / (2.0 * g) \n"
        "        * (1.0 - exp(-g / tan_p))     \n"
        "        * (3.0 - exp(-g / tan_p));    \n"
        "\n"
        // Scattering.
        "    float _S = (sin(p) + (PI - p) * cos_p) / PI \n"
        "        + t * (1.0 - cos_p) * (1.0 - cos_p);\n"
        "\n"
        // BRDF
        "    float F = TWO_OVER_THREEPI * _R * _S * 1.0 / (1.0 + (-dot_ne) / dot_nl);\n"
        "\n"
        "    if(dot_nl > 0.0)\n"
        "        F = 0.0;\n"
        "\n"


        // Fetch Albedo and apply orientation for correct "FrontFacing" with optical librations.
        "    vec3 stu = (vec4(x, y, z, 1.0) * R).xyz;\n"
        "    vec3 c = textureCube(moonCube, stu).xyz;\n"
        "\n"
        "    vec3 diffuse = vec3(0);\n"
        "    diffuse += earthShine;\n"
        "    diffuse += sunShine.w * sunShine.rgb * F;\n"
        "\n"
        "    diffuse *= c;\n"
        "    diffuse  = max(vec3(0.0), diffuse);\n"
        "\n"
            // Day-Twilight-Night-Intensity Mapping (Butterworth-Filter)
        "    float b = 3.8 / sqrt(1 + pow(sun.z + 1.05, 16)) + 0.2;\n"
        "\n"

        // lunar eclipse - raw version -> TODO: move to texture and CPU brightness function

        "    float _e0 = 0.00451900239074503315565337058629;\n"
        "    float _e1 = 4.65 * _e0;\n"
        "    float _e2 = 2.65 * _e0;\n"
        "\n"
        
        // scale to moon size in unitsphere if unit diameter is double earth moon distance
        "    vec3  _a = mn * _e0 - m;\n"
        "    float _d = length(cross(_a, sun));\n"
        "\n"
        "    vec3 le = vec3(1);\n"
        "\n"
        "    if(_d - _e1 < 0)\n"
        "    {\n"
        "        vec3 le0 = 0.600 * vec3(1.0, 1.0, 1.0);\n"
        "        vec3 le1 = 1.800 * vec3(1.0, 1.0, 1.0);\n"
        "        vec3 le2 = 0.077 * vec3(0.5, 0.8, 1.0);\n"
        "        vec3 le3 = 0.050 * vec3(0.3, 0.4, 0.9);\n"
        "\n"
        "        float s2 = 0.08;\n"
        "\n"
        "        le = vec3(1)\n"
        "           - le0 * min(1.0, smoothstep(_e1, _e2, _d))\n"
        "           - le1 * min(0.2, smoothstep(_e2 * (1 + s2), _e2 * (1 - s2), _d));\n"
        "\n"

        // brightness from mean distance

        "        vec3  _a2 = m * _e0 - m; // scale to moon size in unitsphere if unit diameter is double earth moon distance\n"
        "        float _d2 = length(cross(_a2, sun));\n"
        "\n"
        "        float r_x = (1.825 - 0.5 * _d2 / _e0) / 1.825;\n"
        "        float b = 1;\n"
        "\n"
        "        if(r_x > 0.0)\n"
		"        {\n"
        "            b = 1 + 28 * (3 * r_x * r_x - 2 * r_x * r_x * r_x);\n"
        "\n"
        "            if(_d - _e2 * 2 < 0)\n"
        "            {\n"
        "                le -= le2 * clamp(1 - _d / _e2, 0, 1);\n"
        "                le += le3 * smoothstep(_e2 * (1 - s2 * 2), _e2 * (1 + s2), _d);\n"
        "            }\n"
        "        }\n"
        "        le *= b;\n"
        "    }\n"
        "\n"

        "    gl_FragColor = vec4(le * diffuse, 1.0);\n"
        "}");

        // DEBUG: Show penumbar and umbra shadows
        // "    vec3 f = vec3(1)\n"
        // "        - vec3(0.33) * smoothstep(_e1 * (1.0 + 0.01), _e1 * (1.0 - 0.01), _d)\n"
        // "        - vec3(0.33) * smoothstep(_e2 * (1.0 + 0.01), _e2 * (1.0 - 0.01), _d);\n"

        // DEBUG:
        //    gl_FragColor = vec4(x * 0.5 + 0.5, y * 0.5 + 0.5, 0.0, 0.0);
        //    gl_FragColor = vec4(mn * 0.5 + 0.5, 1.0);
        //    gl_FragColor = vec4(n * 0.5 + 0.5, 1.0);
        //    gl_FragColor = vec4(vec3(cd), 1.0);
        //    gl_FragColor = vec4(d * vec3(1.06, 1.06, 0.98), 1.0);
}




#ifdef OSGHIMMEL_EXPOSE_SHADERS

osg::Shader *MoonGeode::getVertexShader()
{
    return m_vShader;
}
osg::Shader *MoonGeode::getGeometryShader()
{
    return NULL;
}
osg::Shader *MoonGeode::getFragmentShader()
{
    return m_fShader;
}

#endif // OSGHIMMEL_EXPOSE_SHADERS

} // namespace osgHimmel