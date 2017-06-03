/*
 * maes.cpp
 *
 *  Created on: 14 May 2017
 *      Author: Carmen Chan-Zheng
 */

#include "maes.h"

namespace MAES{
/*********************************************************************************************
*  Unnamed namespace for using within namespace
**********************************************************************************************/

    namespace{
/*********************************************************************************************
*
*                         Class: AMS_Services
*
*********************************************************************************************
*********************************************************************************************
* Class: AMS_Services
* Function: Agent_Services Constructor
* Comment: Initialize list of agents task handle and its environment to NULL.
**********************************************************************************************/
        AMS_Services::AMS_Services(){
            int i=0;
            AP.next_available=0;
            while (i<AGENT_LIST_SIZE){
                AP.Agent_Handle[i]=(Task_Handle) NULL;
                Task_setEnv(AP.Agent_Handle[i],NULL);
                i++;
            }
        }
/*********************************************************************************************
* Class: AMS_Services
* Function: get_AP()
* Comment: Get information of Agent Platform.
**********************************************************************************************/
        AP_Description*AMS_Services::get_AP(){
            return &AP;
        }
/*********************************************************************************************
* Class: AMS_Services
* Function:  bool search();
* Return: Bool
* Comment: Search AID within list and return true if found.
*          next_available is used instead of AGENT_LIST_SIZE since it will optimize
*          search
**********************************************************************************************/
        bool AMS_Services::search(Task_Handle aid){
            int i=0;

              while(i<AP.next_available){
                  if (AP.Agent_Handle[i]==aid) break;
                  i++;
              }

              if (i==AP.next_available) return false;
              else return true;
       }
/*********************************************************************************************
* Class: AMS_Services
* Function: int register_agent(Task_Handle aid)
* Comment: Register agent to the platform only if it is unique by agent's task
**********************************************************************************************/
        int AMS_Services::register_agent(Task_Handle aid){
            if (aid==NULL) return HANDLE_NULL;

            if (!search(aid)){

                if(AP.next_available<AGENT_LIST_SIZE){
                Agent_info *description;
                description=(Agent_info *)Task_getEnv(aid);
                description->AP=AP.name;
                AP.Agent_Handle[AP.next_available]=aid;
                AP.next_available++;
                Task_setPri(aid, description->priority);
                return NO_ERROR;
                }
                else return LIST_FULL;
            }
            else return DUPLICATED;
        }
/*********************************************************************************************
* Class: AMS_Services
* Function:  int deregister_agent(Task_Handle aid);
* Comment: Deregister agent on the platform by handle. It searches inside of the list, when found,
*          the rest of the list is shifted to the right and the agent is removed.
**********************************************************************************************/
        int AMS_Services::deregister_agent(Task_Handle aid){
            int i=0;
            while(i<AGENT_LIST_SIZE){
                  if(AP.Agent_Handle[i]==aid){
                          Agent_info *description;
                          suspend(aid);
                          description=(Agent_info *)Task_getEnv(aid);
                          description->AP=NULL;

                          while(i<AGENT_LIST_SIZE-1){
                              AP.Agent_Handle[i]=AP.Agent_Handle[i+1];
                          i++;
                      }
                      AP.Agent_Handle[AGENT_LIST_SIZE-1]=NULL;
                      AP.next_available--;
                      break;
                  }
                  i++;
              }

              if (i==AGENT_LIST_SIZE) return NOT_FOUND;
              else return NO_ERROR;
       }
/*********************************************************************************************
* Class: AMS_Services
* Function:  int kill_agent(Task_Handle aid);
* Return: int
* Comment: Kill agent on the platform. It deregisters the agent first then it delete the
*           handles by agent's handle
**********************************************************************************************/
        int AMS_Services::kill_agent(Task_Handle aid){
            int error;
            error=deregister_agent(aid);

            if(error==NO_ERROR){
                Agent_info *description;
                Mailbox_Handle m;
                description=(Agent_info *)Task_getEnv(aid);
                m=description->mailbox_handle;
                Task_delete(&aid);
                Mailbox_delete(&m);
            }

            return error;
        }

/*********************************************************************************************
* Class: AMS_Services
* Function:void suspend(Task_Handle aid)
* Return:  NULL
* Comment: Suspend Agent. Set it to inactive by setting priority to -1
**********************************************************************************************/
        void AMS_Services::suspend(Task_Handle aid){
            if(search(aid)) Task_setPri(aid, -1);
       }
/*********************************************************************************************
* Class: AMS_Services
* Function: bool modify_agent (Task_Handle aid,String new_AP)
* Return:
* Comment: Modifies Agent Platform name of the agent (Used if needed to migrate)
*          Search AID within list and return true if modified correctly
*          Suspend agent when modified
**********************************************************************************************/
        bool AMS_Services::modify_agent(Task_Handle aid,String new_AP){

            if(search(aid)){
                Agent_info *description;
                suspend(aid);
                description=(Agent_info *)Task_getEnv(aid);
                description->AP=new_AP;
                return true;
            }

            else return false;
        }
/*********************************************************************************************
* Class: AMS_Services
* Function:void restore(Agent agent)
* Return:  NULL
* Comment: Restore Agent.
**********************************************************************************************/
       void AMS_Services::resume(Task_Handle aid){
            if(search(aid)) {
                Agent_info *description;
                description=(Agent_info *)Task_getEnv(aid);
                Task_setPri(aid,description->priority);
            }
       }
/*********************************************************************************************
* Class: AMS_Services
* Function: get_AMS_AID();
* Return: Task Handle of the AMS
* Comment: returns if was successful
**********************************************************************************************/
       bool AMS_Services::set_agent_pri(Task_Handle aid,int pri){
            if (search(aid)){
                Agent_info *description=(Agent_info *)Task_getEnv(aid);
                Task_setPri(aid,pri);
                description->priority=pri;
                return true;
            }
            else return false;
        }
/*********************************************************************************************
* Class: AMS_Services
* Function:void get_mode
* Return:  NULL
* Comment: get running mode of agent
**********************************************************************************************/
       int AMS_Services:: get_mode(Task_Handle aid){
           if(search(aid)){
               Task_Mode mode;
               mode=Task_getMode(aid);

               switch(mode){
               case Task_Mode_READY:
                   return ACTIVE;

               case Task_Mode_BLOCKED:
                   return WAITING;

               case Task_Mode_INACTIVE:
                   return SUSPENDED;

               case Task_Mode_TERMINATED:
                   return TERMINATED;

               default: return NULL;

               }
           }
           else return NULL;
       }
/*********************************************************************************************
* Class: AMS_Services
* Function: broadcast(MsgObj &msg)
* Return: NULL
* Comment: broadcast the msg to all subscribers
**********************************************************************************************/
      void AMS_Services::broadcast(MsgObj *msg){
          Agent_info * description;

       //   msg->handle=Task_self(); //Set sender
          int i=0;

          while(i<AP.next_available){
              if(msg->sender_agent!=AP.Agent_Handle[i]){
                  description=(Agent_info*)Task_getEnv(AP.Agent_Handle[i]);
                  Mailbox_post(description->mailbox_handle, (xdc_Ptr)msg, BIOS_NO_WAIT);
              }
              i++;
          }
      }

/*********************************************************************************************
* Class: -
* Function: AMS_task
* Return: NULL
* Comment: AMS task description. No visible outside of namespace. For internal use only
**********************************************************************************************/
        void AMS_task(UArg arg0, UArg arg1){
            Agent_info *agent;
            Mailbox_Handle m;
            Agent_Msg msg;
            agent=(Agent_info *)Task_getEnv(Task_self());
            m=agent->mailbox_handle;
            AMS_Services *services=(AMS_Services*)arg0;


            while(1){
                Mailbox_pend(m, (xdc_Ptr) &msg,BIOS_WAIT_FOREVER);
            }
        }
    }
/*********************************************************************************************
*
*                         Class: Agent_AMS: Agent Management Services
*
********************************************************************************************/
/*********************************************************************************************
* Class: Agent_Platform
* Function: Agent_Platform Constructor
* Comment: Initialize list of agents task handle and its environment to NULL.
**********************************************************************************************/
    Agent_Platform::Agent_Platform(String name){
         AP_Description *ptr=services.get_AP();
         ptr->name=name;
    }
/*********************************************************************************************
* Class: Agent_Platform
* Function: bool init();
* Return: Boolean
* Comment: Create AMS task with default stack of 1024
***********************************************************************************************/
    bool Agent_Platform::init(){

        Task_Handle temp;
        Mailbox_Params mbxParams;
        Task_Params taskParams;
        AP_Description *ptr=services.get_AP();

        /*Initializing Agent_info*/
        description.AP=ptr->name;
        description.agent_name=ptr->name;
        description.priority=Task_numPriorities-1;

        /*Creating mailbox*/
        Mailbox_Params_init(&mbxParams);
        description.mailbox_handle= Mailbox_create(16,5,&mbxParams,NULL);

        /*Creating task*/
        Task_Params_init(&taskParams);
        taskParams.stack=task_stack;
        taskParams.stackSize = 1024;
        taskParams.priority = description.priority;//Assigning max priority
        taskParams.instance->name=description.AP;
        taskParams.env=&description;
        taskParams.arg0=(UArg)&services;
        ptr->AMS_aid = Task_create(AMS_task, &taskParams, NULL);

        if (ptr->AMS_aid!=NULL){
        /*Initializing all the previously created task*/
            temp=Task_Object_first();
            while (temp!=NULL){
                if(temp==ptr->AMS_aid) break;
                services.register_agent(temp);
                //register_agent(temp);
                temp=Task_Object_next(temp);
            }

            return true;
        }
        else return false;
   }

/*********************************************************************************************
* Class: Agent_Platform
* Function: bool init(Task_FuncPtr action,int taskstackSize);;
* Comment: Create AMS task with user custom stack size
*          Be aware of heap size
**********************************************************************************************/
    bool Agent_Platform::init(Task_FuncPtr action,int taskstackSize){

        Task_Handle temp;
        Mailbox_Params mbxParams;
        Task_Params taskParams;
        AP_Description *ptr=services.get_AP();

        /*Initializing Agent_info*/
        description.AP=ptr->name;
        description.agent_name=ptr->name;
        description.priority=Task_numPriorities-1;

        /*Creating mailbox*/
        Mailbox_Params_init(&mbxParams);
        description.mailbox_handle= Mailbox_create(16,5,&mbxParams,NULL);

        /*Creating task*/
        Task_Params_init(&taskParams);
        taskParams.stack=new char[taskstackSize];
        taskParams.stackSize = taskstackSize;
        taskParams.priority = description.priority;//Assigning max priority
        taskParams.instance->name=description.AP;
        taskParams.env=&description;
        ptr->AMS_aid = Task_create(AMS_task, &taskParams, NULL);

        if (ptr->AMS_aid!=NULL){
        /*Initializing all the previously created task*/
            temp=Task_Object_first();
            while (temp!=NULL){
                if(temp==ptr->AMS_aid) break;
                services.register_agent(temp);
                //register_agent(temp);
                temp=Task_Object_next(temp);
            }

            return true;
        }
        else return false;
   }

/*********************************************************************************************
* Class: Agent_Platform
* Function:  bool search(Task_Handle aid);
* Return: Bool
* Comment: search agent in Platform by agent aid
**********************************************************************************************/
    bool Agent_Platform::search(Task_Handle aid){
        return services.search(aid);
   }

/*********************************************************************************************
* Class: Agent_Platform
* Function:  bool search(Agent *a);
* Return: Bool
* Comment: search agent in Platform by Agent object
**********************************************************************************************/
   bool Agent_Platform::search(Agent *a){
        return services.search(a->get_AID());
   }

/*********************************************************************************************
* Class:Agent_Platform
* Function: void agent_wait (uint32 ticks)
* Return type: NULL
* Comments: When called within agent's function it will make agent sleeps defined ticks
*********************************************************************************************/
    void Agent_Platform::agent_wait(Uint32 ticks){
        Task_sleep(ticks);
    }
/*********************************************************************************************
* Class: Agent_Platform
* Function: void agent_yield()
* Return type: NULL
* Comments: It yields the processor to another readied agent of equal priority.
*           It lower priorities are on readied, the current task won't be preempted
*********************************************************************************************/
    void Agent_Platform::agent_yield(){
        Task_yield();
    }
/*********************************************************************************************
* Class:Agent_Platform
* Function: get_running_agent()
* Return type: NULL
* Comments: Returns aid of current running agent
*********************************************************************************************/
    Task_Handle Agent_Platform::get_running_agent_aid(){
        return Task_self();
    }
/*********************************************************************************************
* Class: Agent_Platform
* Function:void get_mode
* Return:  NULL
* Comment: get running mode of agent
**********************************************************************************************/
    int Agent_Platform:: get_mode(Task_Handle aid){
        return services.get_mode(aid);
    }
/*********************************************************************************************
* Class: Agent_Platform
* Function: AP_Description* get_Agent_description(Task_Handle aid);
* Return: Agent_info
* Comment: Returns description of an agent. Returns copy instead of pointer since pointer
*          can override the information.
**********************************************************************************************/
    const Agent_info *Agent_Platform::get_Agent_description(Task_Handle aid){
        Agent_info *description;
        description=(Agent_info *)Task_getEnv(aid);
        return description;
    }
/*********************************************************************************************
* Class: Agent_Platform
* Function: AP_Description* get_Agent_description(Agent *a);
* Return: Agent_info
* Comment: Returns description of an agent. Returns copy instead of pointer since pointer
*          can override the information.
**********************************************************************************************/
    const Agent_info *Agent_Platform::get_Agent_description(Agent *a){
        Agent_info *description;
        description=(Agent_info *)Task_getEnv(a->get_AID());
        return description;

    }
/*********************************************************************************************
* Class: Agent_Platform
* Function: AP_Description* get_AP_description();
* Return: AP_Description
* Comment: Returns description of the platform Returns copy instead of pointer since pointer
*          can override the information.
**********************************************************************************************/
    const AP_Description *Agent_Platform::get_AP_description(){
        return services.get_AP();
    }

/*********************************************************************************************
* Class: Agent_Platform
* Function: get_AMS_AID();
* Return: Task Handle of the AMS
* Comment: returns AMS task handle in case that exists
**********************************************************************************************/
    Task_Handle Agent_Platform::get_AMS_AID(){
        return services.get_AP()->AMS_aid;
    }

/*********************************************************************************************
* Class: Agent_Platform
* Function:  int number_of_subscribers()
* Return: Int
* Comment: Returns size of the list of agents registered in the AP
**********************************************************************************************/
    int Agent_Platform::number_of_subscribers(){
        return services.get_AP()->next_available;
    }
/*********************************************************************************************
*
*                                  Class: Agent
*
**********************************************************************************************
**********************************************************************************************
* Class: Agent
* Function: Agent constructor
**********************************************************************************************/
    Agent::Agent(String name,
                 Task_FuncPtr b){

        description.agent_name=name;
        behaviour=b;
        aid=NULL;
        Task_setEnv(aid,NULL);
    }
/*********************************************************************************************
 * Class: Agent
 * Function: bool init_agent()
 * Return type: Boolean
 * Comments: Create the task and mailbox associated with the agent. The Agent's description
 *           struct is stored in task's environment variable.
 *           Priority set to -1, only set when the agent is registered
*********************************************************************************************/
    bool Agent::init_agent(){

        Task_Params taskParams;
        Mailbox_Params mbxParams;

        description.AP=NULL;
        description.priority= 1;

        /*Creating mailbox: Msg size is 16 and default queue size is set to 3*/
        Mailbox_Params_init(&mbxParams);
        description.mailbox_handle= Mailbox_create(16,5,&mbxParams,NULL);

        /*Creating task*/
        Task_Params_init(&taskParams);
        taskParams.stack=task_stack;
        taskParams.stackSize =1024;
        taskParams.priority = -1;
        taskParams.instance->name=description.agent_name; //To do: take that out to optimize
        taskParams.env=&description;
        aid = Task_create(behaviour, &taskParams, NULL);
        if (aid!=NULL) return true;
        else return false;

    }

 /*********************************************************************************************
 * Class: Agent
 * Function: bool init_agent(int taskstackSize,int queueSize, int priority)
 * Return type: bool
 * Comments: Create the task and mailbox associated with the agent. The Agent's description
 *           struct is stored in task's environment variable.
 *           This init is with user custom's task stack size, queue size and priority.
 *           Be aware of heap size
 *           Priority set to -1, only set when the agent is registered
*********************************************************************************************/
    bool Agent::init_agent(int taskstackSize, int queueSize, int priority){

        Task_Params taskParams;
        Mailbox_Params mbxParams;

        description.AP=NULL;
        description.priority=priority;

        /*Creating mailbox
        * Msg size is 16*/
        Mailbox_Params_init(&mbxParams);
        description.mailbox_handle=Mailbox_create(16,queueSize,&mbxParams,NULL);

        /*Creating task*/
        Task_Params_init(&taskParams);
        taskParams.stack=new char[taskstackSize];
        taskParams.stackSize =taskstackSize;
        taskParams.priority = -1;
        taskParams.instance->name=description.agent_name;
        taskParams.env=&description;
        aid = Task_create(behaviour, &taskParams, NULL);

        if (aid!=NULL) return true;
        else return false;
    }
/*********************************************************************************************
* Class: Agent
* Function: bool init_agent(int taskstackSize,int queueSize, int priority)
* Return type: bool
* Comments: Create the task and mailbox associated with the agent. The Agent's description
*           struct is stored in task's environment variable.
*           This init is with user custom arguments.
*           Priority set to -1, only set when the agent is registered
*********************************************************************************************/
    bool Agent::init_agent(UArg arg0, UArg arg1){

        Task_Params taskParams;
        Mailbox_Params mbxParams;

        description.AP=NULL;
        description.priority=1;

        /*Creating mailbox: Msg size is 16 and default queue size is set to 3*/
        Mailbox_Params_init(&mbxParams);
        description.mailbox_handle= Mailbox_create(16,5,&mbxParams,NULL);

        /*Creating task*/
        Task_Params_init(&taskParams);
        taskParams.stack=task_stack;
        taskParams.stackSize =1024;
        taskParams.priority = -1;
        taskParams.instance->name=description.agent_name; //To do: take that out to optimize
        taskParams.arg0=arg0;
        taskParams.arg1=arg1;
        taskParams.env=&description;
        aid = Task_create(behaviour, &taskParams, NULL);

        if (aid!=NULL) return true;
        else return false;

    }

/*********************************************************************************************
* Class: Agent
* Function: bool init_agent(int taskstackSize,int queueSize, int priority)
* Return type: bool
* Comments: Create the task and mailbox associated with the agent. The Agent's description
*           struct is stored in task's environment variable.
*           This init is with user custom arguments, task stack size, queue size and priority.
*           Be aware of heap size. Priority set to -1, only set when the agent is registered
*********************************************************************************************/
    bool Agent::init_agent(int taskstackSize, int queueSize, int priority,UArg arg0, UArg arg1){

        Task_Params taskParams;
        Mailbox_Params mbxParams;

        description.AP=NULL;
        description.priority=priority;

        /*Creating mailbox
        * Msg size is 16*/
        Mailbox_Params_init(&mbxParams);
        description.mailbox_handle=Mailbox_create(16,queueSize,&mbxParams,NULL);

        /*Creating task*/
        Task_Params_init(&taskParams);
        taskParams.stack=new char[taskstackSize];
        taskParams.stackSize =taskstackSize;
        taskParams.priority =-1;
        taskParams.instance->name=description.agent_name;
        taskParams.arg0=arg0;
        taskParams.arg1=arg1;
        taskParams.env=&description;
        aid = Task_create(behaviour, &taskParams, NULL);

        if (aid!=NULL) return true;
        else return false;
    }

/*********************************************************************************************
* Class: Agent
* Function: void destroy_agent();
* Return type: Boolean
* Comment: Destroy agent if only is de-registered to AP. Returns true if destroyed
*          succesfully
*********************************************************************************************/
    bool Agent::destroy_agent(){
        if (!isRegistered()){
            Task_setEnv(aid,NULL);
            Mailbox_Handle m= description.mailbox_handle;
            Mailbox_delete(&m);
            Task_delete(&aid);
            return true;
        }
        else return false;
    }

/*********************************************************************************************
* Class: Agent
* Function:  get_task_handle(){
* Return type: Task Handle
* Comments: Returns agent's task handle
*********************************************************************************************/
    Task_Handle Agent::get_AID(){
        return aid;
    }

/*********************************************************************************************
* Class: Agent
* Function:  bool isRegistered()
* Return type: NULL
* Comments: Returns true if agent is registered to any platform
*********************************************************************************************/
    bool Agent::isRegistered(){
        if (description.AP!=NULL)return true;
        else return false;
    }
/*********************************************************************************************
*
*                                  Class: Agent_Msg
*
********************************************************************************************
*********************************************************************************************
* Class: Agent_Msg
* Function: Agent_Msg Constructor
* Comment: Construct Agent_Msg Object.
*          Msg object shall be created in the task function, therefore,
*          the Agent_msg object is assigned to the handle of the calling task.
*          The object contains information of the task handle, mailbox and the name
**********************************************************************************************/
   Agent_Msg::Agent_Msg(){
      Agent_info *description=(Agent_info*)Task_getEnv(Task_self());
      self_handle=Task_self();
      self_mailbox=description->mailbox_handle;
      clear_all_receiver();
      next_available=0;

   }
/*********************************************************************************************
* Class: Agent_Msg
* Function: private bool isRegistered(Task_Handle aid);
* Return type: Boolean
* Comment: if returns false, sender or receiver is not registered in the same platform
**********************************************************************************************/
  bool Agent_Msg::isRegistered(Task_Handle aid){
      Agent_info *description_receiver=(Agent_info *)Task_getEnv(aid);
      Agent_info *description_sender=(Agent_info *)Task_getEnv(self_handle);

      if(description_receiver->AP==description_sender->AP) return true;
      else return false;
}
/*********************************************************************************************
* Class: Agent_Msg
* Function: add_receiver(Task_Handle aid)
* Return type: Boolean. True if receiver is added successfully.
* Comment: Add receiver to list of receivers by using the agent's aid
**********************************************************************************************/
   int Agent_Msg::add_receiver(Task_Handle aid_receiver){

        if(isRegistered(aid_receiver)){
           if (aid_receiver==NULL) return HANDLE_NULL;

           if(next_available<MAX_RECEIVERS){
               receivers[next_available]=aid_receiver;
               next_available++;
               return NO_ERROR;
           }

           else return LIST_FULL;

       }
       else return NOT_FOUND;
   }


/*********************************************************************************************
* Class: Agent_Msg
* Function: remove_receiver(Task_Handle aid)
* Return type: Boolean. True if receiver is removed successfully. False if it is not encountered
* Comment: Remove receiver in list of receivers. It searches inside of the list, when found,
* the rest of the list is shifted to the right and the receiver is removed.
**********************************************************************************************/
   int Agent_Msg::remove_receiver(Task_Handle aid){

        int i=0;
        while(i<MAX_RECEIVERS){
            if(receivers[i]==aid){
                while(i<MAX_RECEIVERS-1){
                    receivers[i]=receivers[i+1];
                    i++;
                }
                receivers[MAX_RECEIVERS-1]=NULL;
                next_available--;
                break;
            }
            i++;
        }
        if (i==MAX_RECEIVERS) return NOT_FOUND;
        else return NO_ERROR;
    }
/*********************************************************************************************
* Class: Agent_Msg
* Function: clear_all_receiver();
* Return type: NULL
* Comment: Clear all receiver in the list
**********************************************************************************************/
    void Agent_Msg::clear_all_receiver(){
        int i=0;
        while (i<MAX_RECEIVERS){
            receivers[i]=NULL;
            i++;
        }
    }
/*********************************************************************************************
* Class: Agent_Msg
* Function: refresh_list()
* Return type: NULL
* Comment: Refresh the list with all the registered agents. Remove agent if it is not registered
**********************************************************************************************/
    void Agent_Msg::refresh_list(){
        int i=0;
        while (i<next_available){
            if(!isRegistered(receivers[i]))remove_receiver(receivers[i]);
            i++;
        }
    }
/*********************************************************************************************
* Class: Agent_Msg
* Function: receive_msg(Uint32 timeout)
* Return type: Boolean. TRUE if successful, FALSE if timeout
* Comment: Receiving msg in its queue. Block call. The mailbox is obtained from the
*          task handle of the calling function of this object.
**********************************************************************************************/
    bool Agent_Msg::receive(Uint32 timeout){

        return Mailbox_pend(self_mailbox, (xdc_Ptr) &msg, timeout);
    }

/*********************************************************************************************
* Class: Agent_Msg
* Function: send()
* Return type: Boolean. TRUE if successful, FALSE if timeout
* Comment: Send msg to specific mailbox.
*          Set the MsgObj handle to sender's handle.
**********************************************************************************************/
    int Agent_Msg::send(Task_Handle aid_receiver){
        msg.sender_agent=self_handle;
        Agent_info * description;

        if(isRegistered(aid_receiver)){
            description=(Agent_info*)Task_getEnv(aid_receiver);

            if(Mailbox_post(description->mailbox_handle, (xdc_Ptr)&msg, BIOS_NO_WAIT)) return NO_ERROR;
            else return TIMEOUT;
        }
        else return NOT_FOUND;

    }

/*********************************************************************************************
* Class: Agent_Msg
* Function: send()
* Return type: Boolean. TRUE if all msgs are sent successfully to all receivers
*                       FALSE if there were an error.
* Comment: Iterate over the list. If there is an error for any receiver, will return false
*          if there is any error.
**********************************************************************************************/
     bool Agent_Msg::send(){
        int i=0;
        bool no_error=true;
        msg.sender_agent=self_handle;
        Agent_info * description;

        while (i<next_available){
            if(isRegistered(receivers[i])){
                description=(Agent_info*)Task_getEnv(receivers[i]);
                if(!Mailbox_post(description->mailbox_handle, (xdc_Ptr)&msg, BIOS_NO_WAIT))
                    no_error=false;
            }
            i++;
        }
        return no_error;
    }
/*********************************************************************************************
* Class: Agent_Msg
* Function: set_msg_type(int type)
* Return type: NULL
* Comment: Set message type according to FIPA ACL
**********************************************************************************************/
    void Agent_Msg::set_msg_type(int msg_type){
        msg.type=msg_type;
    }

/*********************************************************************************************
* Class: Agent_Msg
* Function: set_msg_body(String body)
* Return type: NULL
* Comment: Set message body according to FIPA ACL
**********************************************************************************************/
    void Agent_Msg::set_msg_body(String msg_body){
        msg.body=msg_body;
    }
/*********************************************************************************************
* Class: Agent_Msg
* Function: get_msg();
* Return type: MsgObj
* Comment: Get message
**********************************************************************************************/
    MsgObj *Agent_Msg::get_msg(){
        MsgObj *ptr =&msg;
        return ptr;
    }
/*********************************************************************************************
* Class: Agent_Msg
* Function: get_msg_type()
* Return type: int
* Comment: Get message type
**********************************************************************************************/
    int Agent_Msg::get_msg_type(){
        return msg.type;
    }

/*********************************************************************************************
* Class: Agent_Msg
* Function: get_msg_body()
* Return type: String
* Comment: Get message body
**********************************************************************************************/
    String Agent_Msg::get_msg_body(){
        return msg.body;
    }

/*********************************************************************************************
* Class: Agent_Msg
* Function: get_sender()
* Return type: String
* Comment: Get sender name
**********************************************************************************************/
    Task_Handle Agent_Msg::get_sender(){
        return msg.sender_agent;
    }


};

