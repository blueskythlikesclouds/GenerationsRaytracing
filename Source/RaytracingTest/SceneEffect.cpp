#include "SceneEffect.h"

static const tinyxml2::XMLElement* getElement(const tinyxml2::XMLElement* element, const std::initializer_list<const char*>& names)
{
    for (auto& name : names)
    {
        element = element->FirstChildElement(name);
        if (!element) return nullptr;
    }

    return element;
}

static bool getElementValue(const tinyxml2::XMLElement* element, float& value)
{
    if (!element) return false;

    value = element->FloatText(value);
    return true;
}

static bool getElementValue(const tinyxml2::XMLElement* element, uint32_t& value)
{
    if (!element) return false;

    value = element->UnsignedText(value);
    return true;
}

void SceneEffect::LightScattering::computeGpuValues()
{
    gpu.rayMieRay2Mie2.x() = rayleigh;
    gpu.rayMieRay2Mie2.y() = mie;
    gpu.rayMieRay2Mie2.z() = rayleigh * 3.0f / ((float) M_PI * 16.0f);
    gpu.rayMieRay2Mie2.w() = mie / ((float) M_PI * 4.0f);
    gpu.gAndFogDensity.x() = (1.0f - g) * (1.0f - g);
    gpu.gAndFogDensity.y() = g * g + 1.0f;
    gpu.gAndFogDensity.z() = g * -2.0f;
    gpu.farNearScale.x() = 1.0f / (zFar - zNear);
    gpu.farNearScale.y() = zNear;
    gpu.farNearScale.z() = depthScale;
    gpu.farNearScale.w() = inScatteringScale;
}

void SceneEffect::EyeLight::computeGpuValues()
{
    gpu.range.z() = farRangeStart;
    gpu.range.w() = farRangeEnd;

    gpu.attribute.x() = (float)mode;
    gpu.attribute.y() = cos(innerDiameter / 180.0f * (float)M_PI);
    gpu.attribute.z() = cos(outerDiameter / 180.0f * (float)M_PI);
    gpu.attribute.w() = falloff;
}

void SceneEffect::load(const tinyxml2::XMLDocument& doc)
{
    const auto element = doc.FirstChildElement("SceneEffect.prm.xml");
    if (!element)
        return;

    if (const auto hdrElement = getElement(element, { "HDR", "Category", "Basic", "Param" }); hdrElement)
    {
        getElementValue(hdrElement->FirstChildElement("Middle_Gray"), hdr.middleGray);
        getElementValue(hdrElement->FirstChildElement("Luminance_Low"), hdr.lumMin);
        getElementValue(hdrElement->FirstChildElement("Luminance_High"), hdr.lumMax);
    }

    if (const auto defaultElement = getElement(element, { "Default", "Category", "Basic", "Param" }); defaultElement)
        getElementValue(defaultElement->FirstChildElement("CFxSceneRenderer::m_skyIntensityScale"), defaults.skyIntensityScale);

    if (const auto lightScatteringElement = getElement(element, { "LightScattering", "Category" }); lightScatteringElement)
    {
        lightScattering.enable = true;

        if (const auto commonElement = getElement(lightScatteringElement, { "Common", "Param" }); commonElement)
        {
            getElementValue(commonElement->FirstChildElement("ms_Color.x"), lightScattering.color.x());
            getElementValue(commonElement->FirstChildElement("ms_Color.y"), lightScattering.color.y());
            getElementValue(commonElement->FirstChildElement("ms_Color.z"), lightScattering.color.z());
            getElementValue(commonElement->FirstChildElement("ms_FarNearScale.z"), lightScattering.depthScale);
        }

        if (const auto lightScatteringElem = getElement(lightScatteringElement, { "LightScattering", "Param" }); lightScatteringElem)
        {
            getElementValue(lightScatteringElem->FirstChildElement("ms_FarNearScale.w"), lightScattering.inScatteringScale);
            getElementValue(lightScatteringElem->FirstChildElement("ms_Ray_Mie_Ray2_Mie2.x"), lightScattering.rayleigh);
            getElementValue(lightScatteringElem->FirstChildElement("ms_Ray_Mie_Ray2_Mie2.y"), lightScattering.mie);
            getElementValue(lightScatteringElem->FirstChildElement("ms_G"), lightScattering.g);
        }

        if (const auto fogElement = getElement(lightScatteringElement, { "Fog", "Param" }); fogElement)
        {
            getElementValue(fogElement->FirstChildElement("ms_FarNearScale.y"), lightScattering.zNear);
            getElementValue(fogElement->FirstChildElement("ms_FarNearScale.x"), lightScattering.zFar);
        }

        lightScattering.computeGpuValues();
    }

    if (const auto eyeLightElement = getElement(element, { "Light_Manager", "Category", "Basic", "Param" }))
    {
        getElementValue(eyeLightElement->FirstChildElement("EyeLightDiffuse.x"), eyeLight.diffuse.x());
        getElementValue(eyeLightElement->FirstChildElement("EyeLightDiffuse.y"), eyeLight.diffuse.y());
        getElementValue(eyeLightElement->FirstChildElement("EyeLightDiffuse.z"), eyeLight.diffuse.z());

        getElementValue(eyeLightElement->FirstChildElement("EyeLightSpecular.x"), eyeLight.specular.x());
        getElementValue(eyeLightElement->FirstChildElement("EyeLightSpecular.y"), eyeLight.specular.y());
        getElementValue(eyeLightElement->FirstChildElement("EyeLightSpecular.z"), eyeLight.specular.z());

        getElementValue(eyeLightElement->FirstChildElement("EyeLightMode"), eyeLight.mode);
        getElementValue(eyeLightElement->FirstChildElement("EyeLightRange.z"), eyeLight.farRangeStart);
        getElementValue(eyeLightElement->FirstChildElement("EyeLightRange.w"), eyeLight.farRangeEnd);
        getElementValue(eyeLightElement->FirstChildElement("EyeLightInnterDiameter"), eyeLight.innerDiameter);
        getElementValue(eyeLightElement->FirstChildElement("EyeLightOuterDiameter"), eyeLight.outerDiameter);
        getElementValue(eyeLightElement->FirstChildElement("EyeLightAttribute.w"), eyeLight.falloff);

        eyeLight.computeGpuValues();
    }
}
