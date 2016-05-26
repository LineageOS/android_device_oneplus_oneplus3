/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/*
 *  Import and export general routing data using a XML file.
 */
#include "OverrideLog.h"
#include "RouteDataSet.h"
//#include "libxml/xmlmemory.h"
#include <errno.h>
#include <sys/stat.h>

extern char bcm_nfc_location[];


/*******************************************************************************
**
** Function:        AidBuffer
**
** Description:     Parse a string of hex numbers.  Store result in an array of
**                  bytes.
**                  aid: string of hex numbers.
**
** Returns:         None.
**
*******************************************************************************/
AidBuffer::AidBuffer (std::string& aid)
:   mBuffer (NULL),
    mBufferLen (0)
{
    unsigned int num = 0;
    const char delimiter = ':';
    std::string::size_type pos1 = 0;
    std::string::size_type pos2 = aid.find_first_of (delimiter);

    //parse the AID string; each hex number is separated by a colon;
    mBuffer = new UINT8 [aid.length()];
    while (true)
    {
        num = 0;
        if (pos2 == std::string::npos)
        {
            sscanf (aid.substr(pos1).c_str(), "%x", &num);
            mBuffer [mBufferLen] = (UINT8) num;
            mBufferLen++;
            break;
        }
        else
        {
            sscanf (aid.substr(pos1, pos2-pos1+1).c_str(), "%x", &num);
            mBuffer [mBufferLen] = (UINT8) num;
            mBufferLen++;
            pos1 = pos2 + 1;
            pos2 = aid.find_first_of (delimiter, pos1);
        }
    }
}


/*******************************************************************************
**
** Function:        ~AidBuffer
**
** Description:     Release all resources.
**
** Returns:         None.
**
*******************************************************************************/
AidBuffer::~AidBuffer ()
{
    delete [] mBuffer;
}


/*******************************************************************************/
/*******************************************************************************/


const char* RouteDataSet::sConfigFile = "/param/route.xml";


/*******************************************************************************
**
** Function:        ~RouteDataSet
**
** Description:     Release all resources.
**
** Returns:         None.
**
*******************************************************************************/
RouteDataSet::~RouteDataSet ()
{
    deleteDatabase ();
}


/*******************************************************************************
**
** Function:        initialize
**
** Description:     Initialize resources.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool RouteDataSet::initialize ()
{
    static const char fn [] = "RouteDataSet::initialize";
    ALOGD ("%s: enter", fn);
    //check that the libxml2 version in use is compatible
    //with the version the software has been compiled with
    //LIBXML_TEST_VERSION
    ALOGD ("%s: exit; return=true", fn);
    return true;
}


/*******************************************************************************
**
** Function:        deleteDatabase
**
** Description:     Delete all routes stored in all databases.
**
** Returns:         None.
**
*******************************************************************************/
void RouteDataSet::deleteDatabase ()
{
    static const char fn [] = "RouteDataSet::deleteDatabase";
    ALOGD ("%s: default db size=%u; sec elem db size=%u", fn, mDefaultRouteDatabase.size(), mSecElemRouteDatabase.size());
    Database::iterator it;

    for (it = mDefaultRouteDatabase.begin(); it != mDefaultRouteDatabase.end(); it++)
        delete (*it);
    mDefaultRouteDatabase.clear ();

    for (it = mSecElemRouteDatabase.begin(); it != mSecElemRouteDatabase.end(); it++)
        delete (*it);
    mSecElemRouteDatabase.clear ();
}


/*******************************************************************************
**
** Function:        import
**
** Description:     Import data from an XML file.  Fill the databases.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool RouteDataSet::import ()
{
    static const char fn [] = "RouteDataSet::import";
    ALOGD ("%s: enter", fn);
    bool retval = false;
#if 0
    xmlDocPtr doc;
    xmlNodePtr node1;
    std::string strFilename(bcm_nfc_location);
    strFilename += sConfigFile;

    deleteDatabase ();

    doc = xmlParseFile (strFilename.c_str());
    if (doc == NULL)
    {
        ALOGD ("%s: fail parse", fn);
        goto TheEnd;
    }

    node1 = xmlDocGetRootElement (doc);
    if (node1 == NULL)
    {
        ALOGE ("%s: fail root element", fn);
        goto TheEnd;
    }
    ALOGD ("%s: root=%s", fn, node1->name);

    node1 = node1->xmlChildrenNode;
    while (node1) //loop through all elements in <Routes ...
    {
        if (xmlStrcmp(node1->name, (const xmlChar*) "Route")==0)
        {
            xmlChar* value = xmlGetProp (node1, (const xmlChar*) "Type");
            if (value && (xmlStrcmp (value, (const xmlChar*) "SecElemSelectedRoutes") == 0))
            {
                ALOGD ("%s: found SecElemSelectedRoutes", fn);
                xmlNodePtr node2 = node1->xmlChildrenNode;
                while (node2) //loop all elements in <Route Type="SecElemSelectedRoutes" ...
                {
                    if (xmlStrcmp(node2->name, (const xmlChar*) "Proto")==0)
                        importProtocolRoute (node2, mSecElemRouteDatabase);
                    else if (xmlStrcmp(node2->name, (const xmlChar*) "Tech")==0)
                        importTechnologyRoute (node2, mSecElemRouteDatabase);
                    node2 = node2->next;
                } //loop all elements in <Route Type="SecElemSelectedRoutes" ...
            }
            else if (value && (xmlStrcmp (value, (const xmlChar*) "DefaultRoutes") == 0))
            {
                ALOGD ("%s: found DefaultRoutes", fn);
                xmlNodePtr node2 = node1->xmlChildrenNode;
                while (node2) //loop all elements in <Route Type="DefaultRoutes" ...
                {
                    if (xmlStrcmp(node2->name, (const xmlChar*) "Proto")==0)
                        importProtocolRoute (node2, mDefaultRouteDatabase);
                    else if (xmlStrcmp(node2->name, (const xmlChar*) "Tech")==0)
                        importTechnologyRoute (node2, mDefaultRouteDatabase);
                    node2 = node2->next;
                } //loop all elements in <Route Type="DefaultRoutes" ...
            }
            if (value)
                xmlFree (value);
        } //check <Route ...
        node1 = node1->next;
    } //loop through all elements in <Routes ...
    retval = true;

TheEnd:
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    ALOGD ("%s: exit; return=%u", fn, retval);
#endif
    return retval;
}


/*******************************************************************************
**
** Function:        saveToFile
**
** Description:     Save XML data from a string into a file.
**                  routesXml: XML that represents routes.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool RouteDataSet::saveToFile (const char* routesXml)
{
    static const char fn [] = "RouteDataSet::saveToFile";
    FILE* fh = NULL;
    size_t actualWritten = 0;
    bool retval = false;
    std::string filename (bcm_nfc_location);
    int stat = 0;

    filename.append (sConfigFile);
    fh = fopen (filename.c_str (), "w");
    if (fh == NULL)
    {
        ALOGE ("%s: fail to open file", fn);
        return false;
    }

    actualWritten = fwrite (routesXml, sizeof(char), strlen(routesXml), fh);
    retval = actualWritten == strlen(routesXml);
    fclose (fh);
    ALOGD ("%s: wrote %u bytes", fn, actualWritten);
    if (retval == false)
        ALOGE ("%s: error during write", fn);

    //set file permission to
    //owner read, write; group read; other read
    stat = chmod (filename.c_str (), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat == -1)
        ALOGE ("%s: error during chmod", fn);
    return retval;
}


/*******************************************************************************
**
** Function:        loadFromFile
**
** Description:     Load XML data from file into a string.
**                  routesXml: string to receive XML data.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool RouteDataSet::loadFromFile (std::string& routesXml)
{
    static const char fn [] = "RouteDataSet::loadFromFile";
    FILE* fh = NULL;
    size_t actual = 0;
    char buffer [1024];
    std::string filename (bcm_nfc_location);

    filename.append (sConfigFile);
    fh = fopen (filename.c_str (), "r");
    if (fh == NULL)
    {
        ALOGD ("%s: fail to open file", fn);
        return false;
    }

    while (true)
    {
        actual = fread (buffer, sizeof(char), sizeof(buffer), fh);
        if (actual == 0)
            break;
        routesXml.append (buffer, actual);
    }
    fclose (fh);
    ALOGD ("%s: read %u bytes", fn, routesXml.length());
    return true;
}


#if 0


/*******************************************************************************
**
** Function:        importProtocolRoute
**
** Description:     Parse data for protocol routes.
**                  element: XML node for one protocol route.
**                  database: store data in this database.
**
** Returns:         None.
**
*******************************************************************************/
void RouteDataSet::importProtocolRoute (xmlNodePtr& element, Database& database)
{
    static const char fn [] = "RouteDataSet::importProtocolRoute";
    const xmlChar* id = (const xmlChar*) "Id";
    const xmlChar* secElem = (const xmlChar*) "SecElem";
    const xmlChar* trueString = (const xmlChar*) "true";
    const xmlChar* switchOn = (const xmlChar*) "SwitchOn";
    const xmlChar* switchOff = (const xmlChar*) "SwitchOff";
    const xmlChar* batteryOff = (const xmlChar*) "BatteryOff";
    RouteDataForProtocol* data = new RouteDataForProtocol;
    xmlChar* value = NULL;

    ALOGD_IF (sDebug, "%s: element=%s", fn, element->name);
    value = xmlGetProp (element, id);
    if (value)
    {
        if (xmlStrcmp (value, (const xmlChar*) "T1T") == 0)
            data->mProtocol = NFA_PROTOCOL_MASK_T1T;
        else if (xmlStrcmp (value, (const xmlChar*) "T2T") == 0)
            data->mProtocol = NFA_PROTOCOL_MASK_T2T;
        else if (xmlStrcmp (value, (const xmlChar*) "T3T") == 0)
            data->mProtocol = NFA_PROTOCOL_MASK_T3T;
        else if (xmlStrcmp (value, (const xmlChar*) "IsoDep") == 0)
            data->mProtocol = NFA_PROTOCOL_MASK_ISO_DEP;
        xmlFree (value);
        ALOGD_IF (sDebug, "%s: %s=0x%X", fn, id, data->mProtocol);
    }

    value = xmlGetProp (element, secElem);
    if (value)
    {
        data->mNfaEeHandle = strtol ((char*) value, NULL, 16);
        xmlFree (value);
        data->mNfaEeHandle = data->mNfaEeHandle | NFA_HANDLE_GROUP_EE;
        ALOGD_IF (sDebug, "%s: %s=0x%X", fn, secElem, data->mNfaEeHandle);
    }

    value = xmlGetProp (element, switchOn);
    if (value)
    {
        data->mSwitchOn = (xmlStrcmp (value, trueString) == 0);
        xmlFree (value);
    }

    value = xmlGetProp (element, switchOff);
    if (value)
    {
        data->mSwitchOff = (xmlStrcmp (value, trueString) == 0);
        xmlFree (value);
    }

    value = xmlGetProp (element, batteryOff);
    if (value)
    {
        data->mBatteryOff = (xmlStrcmp (value, trueString) == 0);
        xmlFree (value);
    }
    database.push_back (data);
}


/*******************************************************************************
**
** Function:        importTechnologyRoute
**
** Description:     Parse data for technology routes.
**                  element: XML node for one technology route.
**                  database: store data in this database.
**
** Returns:         None.
**
*******************************************************************************/
void RouteDataSet::importTechnologyRoute (xmlNodePtr& element, Database& database)
{
    static const char fn [] = "RouteDataSet::importTechnologyRoute";
    const xmlChar* id = (const xmlChar*) "Id";
    const xmlChar* secElem = (const xmlChar*) "SecElem";
    const xmlChar* trueString = (const xmlChar*) "true";
    const xmlChar* switchOn = (const xmlChar*) "SwitchOn";
    const xmlChar* switchOff = (const xmlChar*) "SwitchOff";
    const xmlChar* batteryOff = (const xmlChar*) "BatteryOff";
    RouteDataForTechnology* data = new RouteDataForTechnology;
    xmlChar* value = NULL;

    ALOGD_IF (sDebug, "%s: element=%s", fn, element->name);
    value = xmlGetProp (element, id);
    if (value)
    {
        if (xmlStrcmp (value, (const xmlChar*) "NfcA") == 0)
            data->mTechnology = NFA_TECHNOLOGY_MASK_A;
        else if (xmlStrcmp (value, (const xmlChar*) "NfcB") == 0)
            data->mTechnology = NFA_TECHNOLOGY_MASK_B;
        else if (xmlStrcmp (value, (const xmlChar*) "NfcF") == 0)
            data->mTechnology = NFA_TECHNOLOGY_MASK_F;
        xmlFree (value);
        ALOGD_IF (sDebug, "%s: %s=0x%X", fn, id, data->mTechnology);
    }

    value = xmlGetProp (element, secElem);
    if (value)
    {
        data->mNfaEeHandle = strtol ((char*) value, NULL, 16);
        xmlFree (value);
        data->mNfaEeHandle = data->mNfaEeHandle | NFA_HANDLE_GROUP_EE;
        ALOGD_IF (sDebug, "%s: %s=0x%X", fn, secElem, data->mNfaEeHandle);
    }

    value = xmlGetProp (element, switchOn);
    if (value)
    {
        data->mSwitchOn = (xmlStrcmp (value, trueString) == 0);
        xmlFree (value);
    }

    value = xmlGetProp (element, switchOff);
    if (value)
    {
        data->mSwitchOff = (xmlStrcmp (value, trueString) == 0);
        xmlFree (value);
    }

    value = xmlGetProp (element, batteryOff);
    if (value)
    {
        data->mBatteryOff = (xmlStrcmp (value, trueString) == 0);
        xmlFree (value);
    }
    database.push_back (data);
}
#endif

/*******************************************************************************
**
** Function:        deleteFile
**
** Description:     Delete route data XML file.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool RouteDataSet::deleteFile ()
{
    static const char fn [] = "RouteDataSet::deleteFile";
    std::string filename (bcm_nfc_location);
    filename.append (sConfigFile);
    int stat = remove (filename.c_str());
    ALOGD ("%s: exit %u", fn, stat==0);
    return stat == 0;
}


/*******************************************************************************
**
** Function:        getDatabase
**
** Description:     Obtain a database of routing data.
**                  selection: which database.
**
** Returns:         Pointer to database.
**
*******************************************************************************/
RouteDataSet::Database* RouteDataSet::getDatabase (DatabaseSelection selection)
{
    switch (selection)
    {
    case DefaultRouteDatabase:
        return &mDefaultRouteDatabase;
    case SecElemRouteDatabase:
        return &mSecElemRouteDatabase;
    }
    return NULL;
}


/*******************************************************************************
**
** Function:        printDiagnostic
**
** Description:     Print some diagnostic output.
**
** Returns:         None.
**
*******************************************************************************/
void RouteDataSet::printDiagnostic ()
{
    static const char fn [] = "RouteDataSet::printDiagnostic";
    Database* db = getDatabase (DefaultRouteDatabase);

    ALOGD ("%s: default route database", fn);
    for (Database::iterator iter = db->begin(); iter != db->end(); iter++)
    {
        RouteData* routeData = *iter;
        switch (routeData->mRouteType)
        {
        case RouteData::ProtocolRoute:
            {
                RouteDataForProtocol* proto = (RouteDataForProtocol*) routeData;
                ALOGD ("%s: ee h=0x%X; protocol=0x%X", fn, proto->mNfaEeHandle, proto->mProtocol);
            }
            break;
        case RouteData::TechnologyRoute:
            {
                RouteDataForTechnology* tech = (RouteDataForTechnology*) routeData;
                ALOGD ("%s: ee h=0x%X; technology=0x%X", fn, tech->mNfaEeHandle, tech->mTechnology);
            }
            break;
        }
    }

    ALOGD ("%s: sec elem route database", fn);
    db = getDatabase (SecElemRouteDatabase);
    for (Database::iterator iter2 = db->begin(); iter2 != db->end(); iter2++)
    {
        RouteData* routeData = *iter2;
        switch (routeData->mRouteType)
        {
        case RouteData::ProtocolRoute:
            {
                RouteDataForProtocol* proto = (RouteDataForProtocol*) routeData;
                ALOGD ("%s: ee h=0x%X; protocol=0x%X", fn, proto->mNfaEeHandle, proto->mProtocol);
            }
            break;
        case RouteData::TechnologyRoute:
            {
                RouteDataForTechnology* tech = (RouteDataForTechnology*) routeData;
                ALOGD ("%s: ee h=0x%X; technology=0x%X", fn, tech->mNfaEeHandle, tech->mTechnology);
            }
            break;
        }
    }
}
