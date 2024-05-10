#ifndef JURACUSTOMMENU_H
#define JURACUSTOMMENU_H
#include "JuraEnums.h"
/*

  name:         JuraCustomMenuItemConfiguration
  type:         array
  description:  this array contains mqtt topics AND ORDER and messages for custom menu items

*/

static const JuraCustomMenuItemConfiguration JuraCustomMenuItemConfigurations[] {
  {
    " RISTR x2",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_espresso",
    "{'add':1,'brew':17}",
  },
  {
    " ADD SHOT ",
    MQTT_ROOT MQTT_DISPENSE_CONFIG,
    "{'add':1,'brew':17}",
  },
  {
    "  CAP +1",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_cappuccino",
    "{'milk':60,'brew':17,'add':1}",
  },
  {
    "CORTADO +1",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_cappuccino",
    "{'milk':30,'brew':17}",
  },
  {
    " AMERI x2",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_hot_water",
    "{'add':2,'brew':17}",
  },
  { /* the last element will always be treated as an exit, regardless the command phrase here */
    "   EXIT",
    "",
    "",
  }
};

#endif