/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 * Ketsji Logic Extension: Network Message Sensor generic implementation
 */

/** \file gameengine/Ketsji/KXNetwork/KX_NetworkMessageSensor.cpp
 *  \ingroup ketsjinet
 */

#include "KX_NetworkMessageSensor.h"

#include <stddef.h>

#include "EXP_InputParser.h"
#include "EXP_ListValue.h"
#include "EXP_StringValue.h"
#include "KX_NetworkMessageScene.h"
#include "SCA_IObject.h"

#ifdef NAN_NET_DEBUG
#  include <iostream>
#endif

KX_NetworkMessageSensor::KX_NetworkMessageSensor(
    SCA_EventManager *eventmgr,                  // our eventmanager
    class KX_NetworkMessageScene *NetworkScene,  // our scene
    SCA_IObject *gameobj,                        // the sensor controlling object
    const std::string &subject)
    : SCA_ISensor(gameobj, eventmgr),
      m_NetworkScene(NetworkScene),
      m_subject(subject),
      m_frame_message_count(0),
      m_BodyList(nullptr),
      m_SubjectList(nullptr)
{
  Init();
}

void KX_NetworkMessageSensor::Init()
{
  m_IsUp = false;
}

KX_NetworkMessageSensor::~KX_NetworkMessageSensor()
{
}

CValue *KX_NetworkMessageSensor::GetReplica()
{
  // This is the standard sensor implementation of GetReplica
  // There may be more network message sensor specific stuff to do here.
  CValue *replica = new KX_NetworkMessageSensor(*this);

  if (replica == nullptr) {
    return nullptr;
  }
  replica->ProcessReplica();

  return replica;
}

/// Return true only for flank (UP and DOWN)
bool KX_NetworkMessageSensor::Evaluate()
{
  bool result = false;
  bool WasUp = m_IsUp;

  m_IsUp = false;

  if (m_BodyList) {
    m_BodyList->Release();
    m_BodyList = nullptr;
  }

  if (m_SubjectList) {
    m_SubjectList->Release();
    m_SubjectList = nullptr;
  }

  std::string toname = GetParent()->GetName();
  std::string &subject = this->m_subject;

  const std::vector<KX_NetworkMessageManager::Message> messages = m_NetworkScene->FindMessages(
      toname, subject);

  m_frame_message_count = messages.size();

  if (!messages.empty()) {
#ifdef NAN_NET_DEBUG
    std::cout << "KX_NetworkMessageSensor found one or more messages" << std::endl;
#endif
    m_IsUp = true;
    m_BodyList = new CListValue<CStringValue>();
    m_SubjectList = new CListValue<CStringValue>();
  }

  std::vector<KX_NetworkMessageManager::Message>::const_iterator mesit;
  for (mesit = messages.begin(); mesit != messages.end(); mesit++) {
    // save the body
    const std::string &body = (*mesit).body;
    // save the subject
    const std::string &messub = (*mesit).subject;
#ifdef NAN_NET_DEBUG
    if (body) {
      cout << "body [" << body << "]\n";
    }
#endif
    m_BodyList->Add(new CStringValue(body, "body"));
    // Store Subject
    m_SubjectList->Add(new CStringValue(messub, "subject"));
  }

  result = (WasUp != m_IsUp);

  // Return always true if a message was received otherwise we can loose messages
  if (m_IsUp)
    return true;
  // Is it useful to return also true when the first frame without a message??
  // This will cause a fast on/off cycle that seems useless!
  return result;
}

/// return true for being up (no flank needed)
bool KX_NetworkMessageSensor::IsPositiveTrigger()
{
  // attempt to fix [ #3809 ] IPO Actuator does not work with some Sensors
  // a better solution is to properly introduce separate Edge and Level triggering concept

  return m_IsUp;
}

#ifdef WITH_PYTHON

/* --------------------------------------------------------------------- */
/* Python interface ---------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* Integration hooks --------------------------------------------------- */
PyTypeObject KX_NetworkMessageSensor::Type = {
    PyVarObject_HEAD_INIT(nullptr, 0) "KX_NetworkMessageSensor",
    sizeof(PyObjectPlus_Proxy),
    0,
    py_base_dealloc,
    0,
    0,
    0,
    0,
    py_base_repr,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Methods,
    0,
    0,
    &SCA_ISensor::Type,
    0,
    0,
    0,
    0,
    0,
    0,
    py_base_new};

PyMethodDef KX_NetworkMessageSensor::Methods[] = {
    {nullptr, nullptr}  // Sentinel
};

PyAttributeDef KX_NetworkMessageSensor::Attributes[] = {
    KX_PYATTRIBUTE_STRING_RW("subject", 0, 100, false, KX_NetworkMessageSensor, m_subject),
    KX_PYATTRIBUTE_INT_RO("frameMessageCount", KX_NetworkMessageSensor, m_frame_message_count),
    KX_PYATTRIBUTE_RO_FUNCTION("bodies", KX_NetworkMessageSensor, pyattr_get_bodies),
    KX_PYATTRIBUTE_RO_FUNCTION("subjects", KX_NetworkMessageSensor, pyattr_get_subjects),
    KX_PYATTRIBUTE_NULL  // Sentinel
};

PyObject *KX_NetworkMessageSensor::pyattr_get_bodies(PyObjectPlus *self_v,
                                                     const KX_PYATTRIBUTE_DEF *attrdef)
{
  KX_NetworkMessageSensor *self = static_cast<KX_NetworkMessageSensor *>(self_v);
  if (self->m_BodyList) {
    return self->m_BodyList->GetProxy();
  }
  else {
    return (new CListValue<CStringValue>())->NewProxy(true);
  }
}

PyObject *KX_NetworkMessageSensor::pyattr_get_subjects(PyObjectPlus *self_v,
                                                       const KX_PYATTRIBUTE_DEF *attrdef)
{
  KX_NetworkMessageSensor *self = static_cast<KX_NetworkMessageSensor *>(self_v);
  if (self->m_SubjectList) {
    return self->m_SubjectList->GetProxy();
  }
  else {
    return (new CListValue<CStringValue>())->NewProxy(true);
  }
}

#endif  // WITH_PYTHON
