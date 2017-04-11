/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (c) 2004-2016 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
/**
 * @file classic_datapath.hpp
 * @brief Base class for top entity for context_switch.
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
*/
#include "top_entity_parallel_cs.hpp"
#include "math.h"
#include "hls.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "hls_manager.hpp"
#include "BambuParameter.hpp"
#include "behavioral_helper.hpp"
#include "technology_node.hpp"
#include "copyrights_strings.hpp"
#include "hls_target.hpp"
#include "technology_manager.hpp"

top_entity_parallel_cs::top_entity_parallel_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type) :
   top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
    debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

top_entity_parallel_cs::~top_entity_parallel_cs()
{

}

DesignFlowStep_Status top_entity_parallel_cs::InternalExec()
{
   /// function name to be synthesized
   structural_objectRef circuit = SM->get_circ();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const std::string function_name = FB->CGetBehavioralHelper()->get_function_name();

   std::string parallel_controller_model = "controller_parallel";
   std::string parallel_controller_name = "controller_parallel";
   std::string par_ctrl_library = HLS->HLS_T->get_technology_manager()->get_library(parallel_controller_model);
   structural_objectRef controller_circuit = SM->add_module_from_technology_library(parallel_controller_name, parallel_controller_model, par_ctrl_library, circuit, HLS->HLS_T->get_technology_manager());

   THROW_ASSERT(HLS->datapath, "Datapath not created");
   //instantiate controller

   HLS->top = structural_managerRef(new structural_manager(parameters));
   SM = HLS->top;
   structural_managerRef Datapath = HLS->datapath;
   structural_managerRef Controller = HLS->controller;

   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(function_name));
   SM->set_top_info(function_name, module_type);
   THROW_ASSERT(circuit, "Top circuit is missing");
   circuit->set_black_box(false);

   ///Set some descriptions and legal stuff
   GetPointer<module>(circuit)->set_description("Top component for " + function_name);
   GetPointer<module>(circuit)->set_copyright(GENERATED_COPYRIGHT);
   GetPointer<module>(circuit)->set_authors("Component automatically generated by bambu");
   GetPointer<module>(circuit)->set_license(GENERATED_LICENSE);

   structural_objectRef datapath_circuit = Datapath->get_circ();

   datapath_circuit->set_owner(circuit);
   GetPointer<module>(circuit)->add_internal_object(datapath_circuit);

   controller_circuit->set_owner(circuit);
   GetPointer<module>(circuit)->add_internal_object(controller_circuit);

   add_port(circuit, controller_circuit, datapath_circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding command ports...");
   this->add_command_signals(circuit);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tCommand ports added!");

   //memory::propagate_memory_parameters(HLS->datapath->get_circ(), HLS->top);
   propagate_memory_signals(Datapath, circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Circuit created without errors!");

   connect_port_parallel(circuit);

   return DesignFlowStep_Status::SUCCESS;
}

void top_entity_parallel_cs::add_port(const structural_objectRef circuit, structural_objectRef controller_circuit, structural_objectRef datapath_circuit)
{
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding clock signal...");
   /// add clock port
   structural_objectRef clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, bool_type);
   GetPointer<port_o>(clock_obj)->set_is_clock(true);
   /// connect to datapath and controller clock
   structural_objectRef datapath_clock = datapath_circuit->find_member(CLOCK_PORT_NAME, port_o_K, datapath_circuit);
   SM->add_connection(datapath_clock, clock_obj);
   structural_objectRef controller_clock = controller_circuit->find_member(CLOCK_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(controller_clock, clock_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tClock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding reset signal...");
   /// add reset port
   structural_objectRef reset_obj = SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, bool_type);
   /// connecting global reset port to the datapath one
   structural_objectRef datapath_reset = datapath_circuit->find_member(RESET_PORT_NAME, port_o_K, datapath_circuit);
   SM->add_connection(datapath_reset, reset_obj);
   /// connecting global reset port to the controller one
   structural_objectRef controller_reset = controller_circuit->find_member(RESET_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(controller_reset, reset_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tReset signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding start signal...");
   /// start port
   structural_objectRef start_obj = SM->add_port(START_PORT_NAME, port_o::IN, circuit, bool_type);
   structural_objectRef controller_start = controller_circuit->find_member(START_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(start_obj, controller_start);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart signal added!");


   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding done signal...");
   /// start port
   structural_objectRef done_obj = SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, bool_type);
   structural_objectRef controller_done = controller_circuit->find_member(DONE_PORT_NAME, port_o_K, controller_circuit);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tDone signal added!");

   const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
   std::string delay_unit;
   std::string synch_reset = HLS->Param->getOption<std::string>(OPT_sync_reset);
   if(synch_reset == "sync")
      delay_unit = flipflop_SR;
   else
      delay_unit = flipflop_AR;
   structural_objectRef delay_gate = SM->add_module_from_technology_library("done_delayed_REG", delay_unit, LIBRARY_STD, circuit, TM);
   structural_objectRef port_ck = delay_gate->find_member(CLOCK_PORT_NAME, port_o_K, delay_gate);
   if(port_ck) SM->add_connection(clock_obj, port_ck);
   structural_objectRef port_rst = delay_gate->find_member(RESET_PORT_NAME, port_o_K, delay_gate);
   if(port_rst) SM->add_connection(reset_obj, port_rst);

   structural_objectRef done_signal_in = SM->add_sign("done_delayed_REG_signal_in", circuit, GetPointer<module>(delay_gate)->get_in_port(2)->get_typeRef());
   SM->add_connection(GetPointer<module>(delay_gate)->get_in_port(2), done_signal_in);
   SM->add_connection(controller_done, done_signal_in);
   structural_objectRef done_signal_out = SM->add_sign("done_delayed_REG_signal_out", circuit, GetPointer<module>(delay_gate)->get_out_port(0)->get_typeRef());
   SM->add_connection(GetPointer<module>(delay_gate)->get_out_port(0), done_signal_out);
   SM->add_connection(done_obj, done_signal_out);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tDone signal added!");

}

void top_entity_parallel_cs::connect_port_parallel(const structural_objectRef circuit)
{
    structural_managerRef Datapath = HLS->datapath;
    structural_managerRef Controller = HLS->controller;
    structural_objectRef datapath_circuit = Datapath->get_circ();
    structural_objectRef controller_circuit = Controller->get_circ();
    structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
    unsigned int num_slots=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
    structural_type_descriptorRef data_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 32));

    structural_objectRef controller_task_finished = controller_circuit->find_member(STR(TASK_FINISHED), port_o_K, controller_circuit);
    structural_objectRef datapath_task_finished = datapath_circuit->find_member(STR(TASK_FINISHED), port_o_K, datapath_circuit);
    structural_objectRef task_finished_sign=SM->add_sign(STR(TASK_FINISHED)+"signal", circuit, bool_type);
    SM->add_connection(datapath_task_finished, task_finished_sign);
    SM->add_connection(task_finished_sign, controller_task_finished);

    structural_objectRef datapath_done_request = datapath_circuit->find_member(STR(DONE_REQUEST)+"accelerator", port_vector_o_K, datapath_circuit);
    structural_objectRef controller_done_request = controller_circuit->find_member(STR(DONE_REQUEST)+"accelerator", port_vector_o_K, datapath_circuit);
    structural_objectRef done_request_sign=SM->add_sign_vector(STR(DONE_REQUEST)+"accelerator"+"signal", num_slots, circuit, bool_type);
    SM->add_connection(datapath_done_request, done_request_sign);
    SM->add_connection(done_request_sign, controller_done_request);

    structural_objectRef datapath_done_port = datapath_circuit->find_member(STR(DONE_PORT_NAME)+"accelerator", port_vector_o_K, datapath_circuit);
    structural_objectRef controller_done_port = controller_circuit->find_member(STR(DONE_PORT_NAME)+"accelerator", port_vector_o_K, controller_circuit);
    structural_objectRef done_port_sign=SM->add_sign_vector(STR(DONE_PORT_NAME)+"accelerator"+"signal", num_slots, circuit, bool_type);
    SM->add_connection(datapath_done_port, done_port_sign);
    SM->add_connection(done_port_sign, controller_done_port);

    structural_objectRef datapath_start_port = datapath_circuit->find_member(STR(START_PORT_NAME)+"accelerator", port_vector_o_K, datapath_circuit);
    structural_objectRef controller_start_port = controller_circuit->find_member(STR(START_PORT_NAME)+"accelerator", port_vector_o_K, controller_circuit);
    structural_objectRef done_start_sign=SM->add_sign_vector(STR(START_PORT_NAME)+"accelerator"+"signal", num_slots, circuit, bool_type);
    SM->add_connection(controller_start_port, done_start_sign);
    SM->add_connection(done_start_sign, datapath_start_port);

    structural_objectRef datapath_request = datapath_circuit->find_member("request_port", port_o_K, datapath_circuit);
    structural_objectRef controller_request = controller_circuit->find_member("request_port", port_o_K, controller_circuit);
    structural_objectRef request_sign=SM->add_sign("request_signal", circuit, data_type);
    SM->add_connection(controller_request, request_sign);
    SM->add_connection(request_sign, datapath_request);

    //connect LoopIteration
}

void top_entity_parallel_cs::propagate_memory_signals(structural_managerRef Datapath, const structural_objectRef circuit)
{
   structural_objectRef cir_port;
   for(unsigned int j = 0; j < GetPointer<module>(Datapath)->get_in_port_size(); j++)
   {
      structural_objectRef port_i = GetPointer<module>(Datapath)->get_in_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
         THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
         if(!cir_port)
         {
            if(port_i->get_kind() == port_vector_o_K)
               cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
            else
               cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
            port_o::fix_port_properties(port_i, cir_port);
            SM->add_connection(cir_port,port_i);
         }
         else
         {
            SM->add_connection(cir_port,port_i);
         }
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(Datapath)->get_out_port_size(); j++)
   {
      structural_objectRef port_i = GetPointer<module>(Datapath)->get_out_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
         THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
         if(!cir_port)
         {
            if(port_i->get_kind() == port_vector_o_K)
               cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
            else
               cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
            port_o::fix_port_properties(port_i, cir_port);
            SM->add_connection(port_i,cir_port);
         }
         else
         {
            SM->add_connection(port_i,cir_port);
         }
      }
   }
}
