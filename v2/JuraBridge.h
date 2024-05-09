#ifndef JURABRIDGE_H
#define JURABRIDGE_H
#include "JuraMachine.h"
#include <ArduinoJson.h>
#include <string>

/* forward declarations */
class Preferences; 
class PubSubClient; 

#define JURA_ENTITY_CONFIGURATION_SIZE 200

class JuraBridge {
  public: 
    /* reminder: need to keep preferences init'd in main *.ino; pass reference here */
    JuraBridge(Preferences &, PubSubClient &, SemaphoreHandle_t &,  SemaphoreHandle_t &);
    
    /* messages from/to machine */
    bool machineStateChanged(JuraMachineStateIdentifier, int);
    bool machineStateStringChanged(JuraMachineStateIdentifier, const char *, int) ;

    /* homeassistant mqtt configuration */
    void publishMachineEntityConfigurations();
    void publishMachineFunctionConfiguration();
    void publishRinseRequest();

    void subscribeToMachineFunctionButtonCommandTopics();
    void subscribeToBridgeSubtopics();

    /* callback for mqtt subscriptions */
    void instructServicePortToSetReady();
    void instructServicePortToDisplayString(String);
    void instructServicePortWithCommand(JuraServicePortCommand);
    void instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier); 

    /* define service port */
    JuraServicePort servicePort;
    const char * getJuraPreferenceKey(JuraMachineStateIdentifier);
    SemaphoreHandle_t &xMQTTSemaphore;

  private: 
    Preferences &preferences;
    PubSubClient &mqttClient;
    SemaphoreHandle_t &xUARTSemaphore;

    /* string handling */
    char* substr(char*, int, int );
  
    /* machine states; not set precisely */
    int _reportableNumericStates[JURA_ENTITY_CONFIGURATION_SIZE]; 
    int _stateIndexToConfigurationIndex[JURA_ENTITY_CONFIGURATION_SIZE]; 

};

#endif
