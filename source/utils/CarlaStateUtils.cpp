/*
 * Carla State utils
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaStateUtils.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

#ifdef HAVE_JUCE_LATER
# include "juce_core.h"
using juce::String;
using juce::XmlElement;
#else
# include <QtCore/QString>
# include <QtXml/QDomNode>
#endif

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// xmlSafeString

#ifdef HAVE_JUCE_LATER
static String xmlSafeString(const String& string, const bool toXml)
{
    String newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}
#else
static QString xmlSafeString(const QString& string, const bool toXml)
{
    QString newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}
#endif

// -----------------------------------------------------------------------
// xmlSafeStringCharDup

#ifdef HAVE_JUCE_LATER
static const char* xmlSafeStringCharDup(const String& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toRawUTF8());
}
#else
static const char* xmlSafeStringCharDup(const QString& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toUtf8().constData());
}
#endif

// -----------------------------------------------------------------------
// StateParameter

StateParameter::StateParameter() noexcept
    : isInput(true),
      index(-1),
      name(nullptr),
      symbol(nullptr),
      value(0.0f),
      midiChannel(0),
      midiCC(-1) {}

StateParameter::~StateParameter() noexcept
{
    if (name != nullptr)
    {
        delete[] name;
        name = nullptr;
    }
    if (symbol != nullptr)
    {
        delete[] symbol;
        symbol = nullptr;
    }
}

// -----------------------------------------------------------------------
// StateCustomData

StateCustomData::StateCustomData() noexcept
    : type(nullptr),
      key(nullptr),
      value(nullptr) {}

StateCustomData::~StateCustomData() noexcept
{
    if (type != nullptr)
    {
        delete[] type;
        type = nullptr;
    }
    if (key != nullptr)
    {
        delete[] key;
        key = nullptr;
    }
    if (value != nullptr)
    {
        delete[] value;
        value = nullptr;
    }
}

// -----------------------------------------------------------------------
// StateSave

StateSave::StateSave() noexcept
    : type(nullptr),
      name(nullptr),
      label(nullptr),
      binary(nullptr),
      uniqueId(0),
      active(false),
      dryWet(1.0f),
      volume(1.0f),
      balanceLeft(-1.0f),
      balanceRight(1.0f),
      panning(0.0f),
      ctrlChannel(-1),
      currentProgramIndex(-1),
      currentProgramName(nullptr),
      currentMidiBank(-1),
      currentMidiProgram(-1),
      chunk(nullptr) {}

StateSave::~StateSave() noexcept
{
    clear();
}

void StateSave::clear() noexcept
{
    if (type != nullptr)
    {
        delete[] type;
        type = nullptr;
    }
    if (name != nullptr)
    {
        delete[] name;
        name = nullptr;
    }
    if (label != nullptr)
    {
        delete[] label;
        label = nullptr;
    }
    if (binary != nullptr)
    {
        delete[] binary;
        binary = nullptr;
    }
    if (currentProgramName != nullptr)
    {
        delete[] currentProgramName;
        currentProgramName = nullptr;
    }
    if (chunk != nullptr)
    {
        delete[] chunk;
        chunk = nullptr;
    }

    uniqueId = 0;
    active = false;
    dryWet = 1.0f;
    volume = 1.0f;
    balanceLeft  = -1.0f;
    balanceRight = 1.0f;
    panning      = 0.0f;
    ctrlChannel  = -1;
    currentProgramIndex = -1;
    currentMidiBank     = -1;
    currentMidiProgram  = -1;

    for (StateParameterItenerator it = parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());
        delete stateParameter;
    }

    for (StateCustomDataItenerator it = customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        delete stateCustomData;
    }

    parameters.clear();
    customData.clear();
}

// -----------------------------------------------------------------------
// fillFromXmlElement

#ifdef HAVE_JUCE_LATER
void StateSave::fillFromXmlElement(const XmlElement* const xmlElement)
{
    clear();

    CARLA_SAFE_ASSERT_RETURN(xmlElement != nullptr,);

    for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        String tagName(elem->getTagName());

        // ---------------------------------------------------------------
        // Info

        if (tagName.equalsIgnoreCase("info"))
        {
            for (XmlElement* xmlInfo = elem->getFirstChildElement(); xmlInfo != nullptr; xmlInfo = xmlInfo->getNextElement())
            {
                const String& tag(xmlInfo->getTagName());
                const String  text(xmlInfo->getAllSubText().trim());

                if (tag.equalsIgnoreCase("type"))
                    type = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("name"))
                    name = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("label") || tag.equalsIgnoreCase("uri"))
                    label = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("binary") || tag.equalsIgnoreCase("bundle") || tag.equalsIgnoreCase("filename"))
                    binary = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("uniqueid"))
                    uniqueId = text.getLargeIntValue();
            }
        }

        // ---------------------------------------------------------------
        // Data

        else if (tagName.equalsIgnoreCase("data"))
        {
            for (XmlElement* xmlData = elem->getFirstChildElement(); xmlData != nullptr; xmlData = xmlData->getNextElement())
            {
                const String& tag(xmlData->getTagName());
                const String  text(xmlData->getAllSubText().trim());

                // -------------------------------------------------------
                // Internal Data

                if (tag.equalsIgnoreCase("active"))
                {
                    active = (text.equalsIgnoreCase("yes") || text.equalsIgnoreCase("true"));
                }
                else if (tag.equalsIgnoreCase("drywet"))
                {
                    dryWet = carla_fixValue(0.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("volume"))
                {
                    volume = carla_fixValue(0.0f, 1.27f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceleft") || tag.equalsIgnoreCase("balance-left"))
                {
                    balanceLeft = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceright") || tag.equalsIgnoreCase("balance-right"))
                {
                    balanceRight = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("panning"))
                {
                    panning = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("controlchannel") || tag.equalsIgnoreCase("control-channel"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1 && value <= MAX_MIDI_CHANNELS)
                        ctrlChannel = static_cast<int8_t>(value-1);
                }

                // -------------------------------------------------------
                // Program (current)

                else if (tag.equalsIgnoreCase("currentprogramindex") || tag.equalsIgnoreCase("current-program-index"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentProgramIndex = value-1;
                }
                else if (tag.equalsIgnoreCase("currentprogramname") || tag.equalsIgnoreCase("current-program-name"))
                {
                    currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag.equalsIgnoreCase("currentmidibank") || tag.equalsIgnoreCase("current-midi-bank"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentMidiBank = value-1;
                }
                else if (tag.equalsIgnoreCase("currentmidiprogram") || tag.equalsIgnoreCase("current-midi-program"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentMidiProgram = value-1;
                }

                // -------------------------------------------------------
                // Parameters

                else if (tag.equalsIgnoreCase("parameter"))
                {
                    StateParameter* const stateParameter(new StateParameter());

                    for (XmlElement* xmlSubData = xmlData->getFirstChildElement(); xmlSubData != nullptr; xmlSubData = xmlSubData->getNextElement())
                    {
                        const String& pTag(xmlSubData->getTagName());
                        const String  pText(xmlSubData->getAllSubText().trim());

                        if (pTag.equalsIgnoreCase("index"))
                        {
                            const int index(pText.getIntValue());
                            if (index >= 0)
                                stateParameter->index = index;
                        }
                        else if (pTag.equalsIgnoreCase("name"))
                        {
                            stateParameter->name = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.equalsIgnoreCase("symbol"))
                        {
                            stateParameter->symbol = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.equalsIgnoreCase("value"))
                        {
                            stateParameter->value = pText.getFloatValue();
                        }
                        else if (pTag.equalsIgnoreCase("midichannel") || pTag.equalsIgnoreCase("midi-channel"))
                        {
                            const int channel(pText.getIntValue());
                            if (channel >= 1 && channel <= MAX_MIDI_CHANNELS)
                                stateParameter->midiChannel = static_cast<uint8_t>(channel-1);
                        }
                        else if (pTag.equalsIgnoreCase("midicc") || pTag.equalsIgnoreCase("midi-cc"))
                        {
                            const int cc(pText.getIntValue());
                            if (cc >= 1 && cc < 0x5F)
                                stateParameter->midiCC = static_cast<int16_t>(cc);
                        }
                    }

                    parameters.append(stateParameter);
                }

                // -------------------------------------------------------
                // Custom Data

                else if (tag.equalsIgnoreCase("customdata") || tag.equalsIgnoreCase("custom-data"))
                {
                    StateCustomData* const stateCustomData(new StateCustomData());

                    for (XmlElement* xmlSubData = xmlData->getFirstChildElement(); xmlSubData != nullptr; xmlSubData = xmlSubData->getNextElement())
                    {
                        const String& cTag(xmlSubData->getTagName());
                        const String  cText(xmlSubData->getAllSubText().trim());

                        if (cTag.equalsIgnoreCase("type"))
                            stateCustomData->type = xmlSafeStringCharDup(cText, false);
                        else if (cTag.equalsIgnoreCase("key"))
                            stateCustomData->key = xmlSafeStringCharDup(cText, false);
                        else if (cTag.equalsIgnoreCase("value"))
                            stateCustomData->value = xmlSafeStringCharDup(cText, false);
                    }

                    customData.append(stateCustomData);
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag.equalsIgnoreCase("chunk"))
                {
                    chunk = xmlSafeStringCharDup(text, false);
                }
            }
        }
    }
}
#else
void StateSave::fillFromXmlNode(const QDomNode& xmlNode)
{
    clear();

    CARLA_SAFE_ASSERT_RETURN(! xmlNode.isNull(),);

    for (QDomNode node = xmlNode.firstChild(); ! node.isNull(); node = node.nextSibling())
    {
        QString tagName(node.toElement().tagName());

        // ---------------------------------------------------------------
        // Info

        if (tagName.compare("info", Qt::CaseInsensitive) == 0)
        {
            for (QDomNode xmlInfo = node.toElement().firstChild(); ! xmlInfo.isNull(); xmlInfo = xmlInfo.nextSibling())
            {
                const QString tag(xmlInfo.toElement().tagName());
                const QString text(xmlInfo.toElement().text().trimmed());

                if (tag.compare("type", Qt::CaseInsensitive) == 0)
                {
                    type = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("name", Qt::CaseInsensitive) == 0)
                {
                    name = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("label", Qt::CaseInsensitive) == 0 || tag.compare("uri", Qt::CaseInsensitive) == 0)
                {
                    label = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("binary", Qt::CaseInsensitive) == 0 || tag.compare("bundle", Qt::CaseInsensitive) == 0 || tag.compare("filename", Qt::CaseInsensitive) == 0)
                {
                    binary = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("uniqueid", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const qlonglong uniqueIdTry(text.toLongLong(&ok));
                    if (ok) uniqueId = static_cast<int64_t>(uniqueIdTry);
                }
            }
        }

        // ---------------------------------------------------------------
        // Data

        else if (tagName.compare("data", Qt::CaseInsensitive) == 0)
        {
            for (QDomNode xmlData = node.toElement().firstChild(); ! xmlData.isNull(); xmlData = xmlData.nextSibling())
            {
                const QString tag(xmlData.toElement().tagName());
                const QString text(xmlData.toElement().text().trimmed());

                // -------------------------------------------------------
                // Internal Data

                if (tag.compare("active", Qt::CaseInsensitive) == 0)
                {
                    active = (text.compare("yes", Qt::CaseInsensitive) == 0 || text.compare("true", Qt::CaseInsensitive) == 0);
                }
                else if (tag.compare("drywet", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) dryWet = carla_fixValue(0.0f, 1.0f, value);
                }
                else if (tag.compare("volume", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) volume = carla_fixValue(0.0f, 1.27f, value);
                }
                else if (tag.compare("balanceleft", Qt::CaseInsensitive) == 0 || tag.compare("balance-left", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) balanceLeft = carla_fixValue(-1.0f, 1.0f, value);
                }
                else if (tag.compare("balanceright", Qt::CaseInsensitive) == 0 || tag.compare("balance-right", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) balanceRight = carla_fixValue(-1.0f, 1.0f, value);
                }
                else if (tag.compare("panning", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) panning = carla_fixValue(-1.0f, 1.0f, value);
                }
                else if (tag.compare("controlchannel", Qt::CaseInsensitive) == 0 || tag.compare("control-channel", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const short value(text.toShort(&ok));
                    if (ok && value >= 1 && value <= MAX_MIDI_CHANNELS)
                        ctrlChannel = static_cast<int8_t>(value-1);
                }

                // -------------------------------------------------------
                // Program (current)

                else if (tag.compare("currentprogramindex", Qt::CaseInsensitive) == 0 || tag.compare("current-program-index", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const int value(text.toInt(&ok));
                    if (ok && value >= 1)
                        currentProgramIndex = value-1;
                }
                else if (tag.compare("currentprogramname", Qt::CaseInsensitive) == 0 || tag.compare("current-program-name", Qt::CaseInsensitive) == 0)
                {
                    currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag.compare("currentmidibank", Qt::CaseInsensitive) == 0 || tag.compare("current-midi-bank", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const int value(text.toInt(&ok));
                    if (ok && value >= 1)
                        currentMidiBank = value-1;
                }
                else if (tag.compare("currentmidiprogram", Qt::CaseInsensitive) == 0 || tag.compare("current-midi-program", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const int value(text.toInt(&ok));
                    if (ok && value >= 1)
                        currentMidiProgram = value-1;
                }

                // -------------------------------------------------------
                // Parameters

                else if (tag.compare("parameter", Qt::CaseInsensitive) == 0)
                {
                    StateParameter* const stateParameter(new StateParameter());

                    for (QDomNode xmlSubData = xmlData.toElement().firstChild(); ! xmlSubData.isNull(); xmlSubData = xmlSubData.nextSibling())
                    {
                        const QString pTag(xmlSubData.toElement().tagName());
                        const QString pText(xmlSubData.toElement().text().trimmed());

                        if (pTag.compare("index", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const int index(pText.toInt(&ok));
                            if (ok && index >= 0) stateParameter->index = index;
                        }
                        else if (pTag.compare("name", Qt::CaseInsensitive) == 0)
                        {
                            stateParameter->name = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.compare("symbol", Qt::CaseInsensitive) == 0)
                        {
                            stateParameter->symbol = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.compare("value", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const float value(pText.toFloat(&ok));
                            if (ok) stateParameter->value = value;
                        }
                        else if (pTag.compare("midichannel", Qt::CaseInsensitive) == 0 || pTag.compare("midi-channel", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const ushort channel(pText.toUShort(&ok));
                            if (ok && channel >= 1 && channel <= MAX_MIDI_CHANNELS)
                                stateParameter->midiChannel = static_cast<uint8_t>(channel-1);
                        }
                        else if (pTag.compare("midicc", Qt::CaseInsensitive) == 0 || pTag.compare("midi-cc", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const int cc(pText.toInt(&ok));
                            if (ok && cc >= 1 && cc < 0x5F)
                                stateParameter->midiCC = static_cast<int16_t>(cc);
                        }
                    }

                    parameters.append(stateParameter);
                }

                // -------------------------------------------------------
                // Custom Data

                else if (tag.compare("customdata", Qt::CaseInsensitive) == 0 || tag.compare("custom-data", Qt::CaseInsensitive) == 0)
                {
                    StateCustomData* const stateCustomData(new StateCustomData());

                    for (QDomNode xmlSubData = xmlData.toElement().firstChild(); ! xmlSubData.isNull(); xmlSubData = xmlSubData.nextSibling())
                    {
                        const QString cTag(xmlSubData.toElement().tagName());
                        const QString cText(xmlSubData.toElement().text().trimmed());

                        if (cTag.compare("type", Qt::CaseInsensitive) == 0)
                            stateCustomData->type = xmlSafeStringCharDup(cText, false);
                        else if (cTag.compare("key", Qt::CaseInsensitive) == 0)
                            stateCustomData->key = xmlSafeStringCharDup(cText, false);
                        else if (cTag.compare("value", Qt::CaseInsensitive) == 0)
                            stateCustomData->value = xmlSafeStringCharDup(cText, false);
                    }

                    customData.append(stateCustomData);
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag.compare("chunk", Qt::CaseInsensitive) == 0)
                {
                    chunk = xmlSafeStringCharDup(text, false);
                }
            }
        }
    }
}
#endif

// -----------------------------------------------------------------------
// fillXmlStringFromStateSave

#ifdef HAVE_JUCE_LATER
String StateSave::toString() const
{
    String content;

    {
        String infoXml("  <Info>\n");

        infoXml << "   <Type>" << String(type != nullptr ? type : "") << "</Type>\n";
        infoXml << "   <Name>" << xmlSafeString(name, true) << "</Name>\n";

        switch (getPluginTypeFromString(type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            infoXml << "    <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_LADSPA:
            infoXml << "    <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "    <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            infoXml << "    <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_DSSI:
            infoXml << "    <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "    <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_LV2:
            infoXml << "    <Bundle>"   << xmlSafeString(binary, true) << "</Bundle>\n";
            infoXml << "    <URI>"      << xmlSafeString(label, true)  << "</URI>\n";
            break;
        case PLUGIN_VST:
            infoXml << "    <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "    <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_VST3:
            // TODO?
            infoXml << "    <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "    <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_AU:
            // TODO?
            infoXml << "    <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "    <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_JACK:
            infoXml << "    <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            break;
        case PLUGIN_REWIRE:
            infoXml << "    <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_FILE_CSD:
        case PLUGIN_FILE_GIG:
        case PLUGIN_FILE_SF2:
        case PLUGIN_FILE_SFZ:
            infoXml << "    <Filename>"   << xmlSafeString(binary, true) << "</Filename>\n";
            infoXml << "    <Label>"      << xmlSafeString(label, true)  << "</Label>\n";
            break;
        }

        infoXml << "  </Info>\n\n";

        content << infoXml;
    }

    content << "  <Data>\n";

    {
        String dataXml;

        dataXml << "   <Active>" << (active ? "Yes" : "No") << "</Active>\n";

        if (dryWet != 1.0f)
            dataXml << "   <DryWet>"        << dryWet       << "</DryWet>\n";
        if (volume != 1.0f)
            dataXml << "   <Volume>"        << volume       << "</Volume>\n";
        if (balanceLeft != -1.0f)
            dataXml << "   <Balance-Left>"  << balanceLeft  << "</Balance-Left>\n";
        if (balanceRight != 1.0f)
            dataXml << "   <Balance-Right>" << balanceRight << "</Balance-Right>\n";
        if (panning != 0.0f)
            dataXml << "   <Panning>"       << panning      << "</Panning>\n";

        if (ctrlChannel < 0)
            dataXml << "   <ControlChannel>N</ControlChannel>\n";
        else
            dataXml << "   <ControlChannel>" << int(ctrlChannel+1) << "</ControlChannel>\n";

        content << dataXml;
    }

    for (StateParameterItenerator it = parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());

        String parameterXml("\n""   <Parameter>\n");

        parameterXml << "    <Index>" << String(stateParameter->index)             << "</Index>\n";
        parameterXml << "    <Name>"  << xmlSafeString(stateParameter->name, true) << "</Name>\n";

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameterXml << "    <Symbol>" << xmlSafeString(stateParameter->symbol, true) << "</Symbol>\n";

        if (stateParameter->isInput)
            parameterXml << "    <Value>" << stateParameter->value << "</Value>\n";

        if (stateParameter->midiCC > 0)
        {
            parameterXml << "    <MidiCC>"      << stateParameter->midiCC        << "</MidiCC>\n";
            parameterXml << "    <MidiChannel>" << stateParameter->midiChannel+1 << "</MidiChannel>\n";
        }

        parameterXml << "   </Parameter>\n";

        content << parameterXml;
    }

    if (currentProgramIndex >= 0 && currentProgramName != nullptr && currentProgramName[0] != '\0')
    {
        // ignore 'default' program
        if (currentProgramIndex > 0 || ! String(currentProgramName).equalsIgnoreCase("default"))
        {
            String programXml("\n");
            programXml << "   <CurrentProgramIndex>" << currentProgramIndex+1                   << "</CurrentProgramIndex>\n";
            programXml << "   <CurrentProgramName>"  << xmlSafeString(currentProgramName, true) << "</CurrentProgramName>\n";

            content << programXml;
        }
    }

    if (currentMidiBank >= 0 && currentMidiProgram >= 0)
    {
        String midiProgramXml("\n");
        midiProgramXml << "   <CurrentMidiBank>"    << currentMidiBank+1    << "</CurrentMidiBank>\n";
        midiProgramXml << "   <CurrentMidiProgram>" << currentMidiProgram+1 << "</CurrentMidiProgram>\n";

        content << midiProgramXml;
    }

    for (StateCustomDataItenerator it = customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->type  != nullptr && stateCustomData->type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->key   != nullptr && stateCustomData->key[0]  != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->value != nullptr);

        String customDataXml("\n""   <CustomData>\n");
        customDataXml << "    <Type>" << xmlSafeString(stateCustomData->type, true) << "</Type>\n";
        customDataXml << "    <Key>"  << xmlSafeString(stateCustomData->key, true)  << "</Key>\n";

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_TYPE_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
        {
            customDataXml << "    <Value>\n";
            customDataXml << xmlSafeString(stateCustomData->value, true);
            customDataXml << "    </Value>\n";
        }
        else
        {
            customDataXml << "    <Value>";
            customDataXml << xmlSafeString(stateCustomData->value, true);
            customDataXml << "</Value>\n";
        }

        customDataXml << "   </CustomData>\n";

        content << customDataXml;
    }

    if (chunk != nullptr && chunk[0] != '\0')
    {
        String chunkXml("\n""   <Chunk>\n");
        chunkXml << chunk << "\n   </Chunk>\n";

        content << chunkXml;
    }

    content << "  </Data>\n";

    return content;
}
#else
QString StateSave::toString() const
{
    QString content;

    {
        QString infoXml("  <Info>\n");

        infoXml += QString("   <Type>%1</Type>\n").arg((type != nullptr) ? type : "");
        infoXml += QString("   <Name>%1</Name>\n").arg(xmlSafeString(name, true));

        switch (getPluginTypeFromString(type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            infoXml += QString("   <Label>%1</Label>\n").arg(xmlSafeString(label, true));
            break;
        case PLUGIN_LADSPA:
            infoXml += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <Label>%1</Label>\n").arg(xmlSafeString(label, true));
            infoXml += QString("   <UniqueID>%1</UniqueID>\n").arg(uniqueId);
            break;
        case PLUGIN_DSSI:
            infoXml += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <Label>%1</Label>\n").arg(xmlSafeString(label, true));
            break;
        case PLUGIN_LV2:
            infoXml += QString("   <Bundle>%1</Bundle>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <URI>%1</URI>\n").arg(xmlSafeString(label, true));
            break;
        case PLUGIN_VST:
            infoXml += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <UniqueID>%1</UniqueID>\n").arg(uniqueId);
            break;
        case PLUGIN_VST3:
            // TODO?
            infoXml += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <UniqueID>%1</UniqueID>\n").arg(uniqueId);
            break;
        case PLUGIN_AU:
            // TODO?
            infoXml += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <UniqueID>%1</UniqueID>\n").arg(uniqueId);
            break;
        case PLUGIN_JACK:
            infoXml += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(binary, true));
            break;
        case PLUGIN_REWIRE:
            infoXml += QString("   <Label>%1</Label>\n").arg(xmlSafeString(label, true));
            break;
        case PLUGIN_FILE_CSD:
        case PLUGIN_FILE_GIG:
        case PLUGIN_FILE_SF2:
        case PLUGIN_FILE_SFZ:
            infoXml += QString("   <Filename>%1</Filename>\n").arg(xmlSafeString(binary, true));
            infoXml += QString("   <Label>%1</Label>\n").arg(xmlSafeString(label, true));
            break;
        }

        infoXml += "  </Info>\n\n";

        content += infoXml;
    }

    content += "  <Data>\n";

    {
        QString dataXml;

        dataXml += QString("   <Active>%1</Active>\n").arg(active ? "Yes" : "No");

        if (dryWet != 1.0f)
            dataXml += QString("   <DryWet>%1</DryWet>\n").arg(dryWet, 0, 'g', 7);
        if (volume != 1.0f)
            dataXml += QString("   <Volume>%1</Volume>\n").arg(volume, 0, 'g', 7);
        if (balanceLeft != -1.0f)
            dataXml += QString("   <Balance-Left>%1</Balance-Left>\n").arg(balanceLeft, 0, 'g', 7);
        if (balanceRight != 1.0f)
            dataXml += QString("   <Balance-Right>%1</Balance-Right>\n").arg(balanceRight, 0, 'g', 7);
        if (panning != 0.0f)
            dataXml += QString("   <Panning>%1</Panning>\n").arg(panning, 0, 'g', 7);

        if (ctrlChannel < 0)
            dataXml += QString("   <ControlChannel>N</ControlChannel>\n");
        else
            dataXml += QString("   <ControlChannel>%1</ControlChannel>\n").arg(ctrlChannel+1);

        content += dataXml;
    }

    for (StateParameterItenerator it = parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());

        QString parameterXml("\n""   <Parameter>\n");

        parameterXml += QString("    <Index>%1</Index>\n").arg(stateParameter->index);
        parameterXml += QString("    <Name>%1</Name>\n").arg(xmlSafeString(stateParameter->name, true));

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameterXml += QString("    <Symbol>%1</Symbol>\n").arg(xmlSafeString(stateParameter->symbol, true));

        if (stateParameter->isInput)
            parameterXml += QString("    <Value>%1</Value>\n").arg(stateParameter->value, 0, 'g', 15);

        if (stateParameter->midiCC > 0)
        {
            parameterXml += QString("    <MidiCC>%1</MidiCC>\n").arg(stateParameter->midiCC);
            parameterXml += QString("    <MidiChannel>%1</MidiChannel>\n").arg(stateParameter->midiChannel+1);
        }

        parameterXml += "   </Parameter>\n";

        content += parameterXml;
    }

    if (currentProgramIndex >= 0 && currentProgramName != nullptr && currentProgramName[0] != '\0')
    {
        // ignore 'default' program
        if (currentProgramIndex > 0 || QString(currentProgramName).compare("default", Qt::CaseInsensitive) != 0)
        {
            QString programXml("\n");
            programXml += QString("   <CurrentProgramIndex>%1</CurrentProgramIndex>\n").arg(currentProgramIndex+1);
            programXml += QString("   <CurrentProgramName>%1</CurrentProgramName>\n").arg(xmlSafeString(currentProgramName, true));

            content += programXml;
        }
    }

    if (currentMidiBank >= 0 && currentMidiProgram >= 0)
    {
        QString midiProgramXml("\n");
        midiProgramXml += QString("   <CurrentMidiBank>%1</CurrentMidiBank>\n").arg(currentMidiBank+1);
        midiProgramXml += QString("   <CurrentMidiProgram>%1</CurrentMidiProgram>\n").arg(currentMidiProgram+1);

        content += midiProgramXml;
    }

    for (StateCustomDataItenerator it = customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->type  != nullptr && stateCustomData->type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->key   != nullptr && stateCustomData->key[0]  != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->value != nullptr);

        QString customDataXml("\n""   <CustomData>\n");
        customDataXml += QString("    <Type>%1</Type>\n").arg(xmlSafeString(stateCustomData->type, true));
        customDataXml += QString("    <Key>%1</Key>\n").arg(xmlSafeString(stateCustomData->key, true));

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_TYPE_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
        {
            customDataXml += "    <Value>\n";
            customDataXml += QString("%1\n").arg(xmlSafeString(stateCustomData->value, true));
            customDataXml += "    </Value>\n";
        }
        else
            customDataXml += QString("    <Value>%1</Value>\n").arg(xmlSafeString(stateCustomData->value, true));

        customDataXml += "   </CustomData>\n";

        content += customDataXml;
    }

    if (chunk != nullptr && chunk[0] != '\0')
    {
        QString chunkXml("\n""   <Chunk>\n");
        chunkXml += QString("%1\n").arg(chunk);
        chunkXml += "   </Chunk>\n";

        content += chunkXml;
    }

    content += "  </Data>\n";

    return content;
}
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
