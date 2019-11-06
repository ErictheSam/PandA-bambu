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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file classic_datapath.cpp
 * @brief Base class for usual datapath creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "classic_datapath.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

#include "commandport_obj.hpp"
#include "dataport_obj.hpp"
#include "generic_obj.hpp"
#include "mux_conn.hpp"
#include "mux_obj.hpp"

#include "technology_manager.hpp"

#include "BambuParameter.hpp"
#include "conn_binding.hpp"
#include "fu_binding.hpp"
#include "memory.hpp"
#include "reg_binding.hpp"
#include "schedule.hpp"
#include "state_transition_graph_manager.hpp"

#include "exceptions.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

#include <boost/lexical_cast.hpp>

/// STD includes
#include <iosfwd>
#include <string>

/// STL includes
#include "custom_map.hpp"
#include <list>

/// technology/physical_library include
#include "technology_node.hpp"

/// utility includes
#include "copyrights_strings.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

classic_datapath::classic_datapath(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : datapath_creator(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

classic_datapath::~classic_datapath() = default;

DesignFlowStep_Status classic_datapath::InternalExec()
{
   /// Test on previous steps. They checks if schedule and connection binding have been performed. If they didn't,
   /// circuit cannot be created.
   THROW_ASSERT(HLS->Rfu, "Functional units not allocated");
   THROW_ASSERT(HLS->Rreg, "Register allocation not performed");
   THROW_ASSERT(HLS->Rconn, "Connection allocation not performed");
   /// Test on memory allocation
   THROW_ASSERT(HLSMgr->Rmem, "Memory allocation not performed");

   /// main circuit type
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor("datapath_" + FB->CGetBehavioralHelper()->get_function_name()));

   /// top circuit creation
   HLS->datapath = structural_managerRef(new structural_manager(HLS->Param));

   HLS->datapath->set_top_info("Datapath_i", module_type);
   const structural_objectRef datapath_cir = HLS->datapath->get_circ();

   // Now the top circuit is created, just as an empty box. <circuit> is a reference to the structural object that
   // will contain all the circuit components

   datapath_cir->set_black_box(false);

   /// Set some descriptions and legal stuff
   GetPointer<module>(datapath_cir)->set_description("Datapath RTL description for " + FB->CGetBehavioralHelper()->get_function_name());
   GetPointer<module>(datapath_cir)->set_copyright(GENERATED_COPYRIGHT);
   GetPointer<module>(datapath_cir)->set_authors("Component automatically generated by bambu");
   GetPointer<module>(datapath_cir)->set_license(GENERATED_LICENSE);

   /// add clock and reset to circuit. It increments in_port number and update in_port_map
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding clock and reset ports");
   structural_objectRef clock, reset;
   add_clock_reset(clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// add all input ports
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding ports for primary inputs and outputs");
   add_ports();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// add registers, connecting them to clock and reset ports
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding registers");
   HLS->Rreg->add_to_SM(clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// allocate functional units
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding functional units");
   HLS->Rfu->add_to_SM(HLSMgr, HLS, clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding multi-unbounded controllers");
   HLS->STG->add_to_SM(clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// allocate interconnections
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding interconnections");
   HLS->Rconn->add_to_SM(HLSMgr, HLS, HLS->datapath);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
   unsigned int n_elements = GetPointer<module>(datapath_cir)->get_internal_objects_size();
   if(n_elements == 0)
   {
      structural_objectRef dummy_gate = HLS->datapath->add_module_from_technology_library("dummy_REG", flipflop_SR, LIBRARY_STD, datapath_cir, HLS->HLS_T->get_technology_manager());
      structural_objectRef port_ck = dummy_gate->find_member(CLOCK_PORT_NAME, port_o_K, dummy_gate);
      if(port_ck)
         HLS->datapath->add_connection(clock, port_ck);
      structural_objectRef port_rst = dummy_gate->find_member(RESET_PORT_NAME, port_o_K, dummy_gate);
      if(port_rst)
         HLS->datapath->add_connection(reset, port_rst);
   }
   /// circuit is now complete. circuit manager can be initialized and dot representation can be created
   HLS->datapath->INIT(true);
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      HLS->datapath->WriteDot(FB->CGetBehavioralHelper()->get_function_name() + "/HLS_Datapath.dot", structural_manager::COMPLETE_G);
   }
   return DesignFlowStep_Status::SUCCESS;
}

void classic_datapath::add_clock_reset(structural_objectRef& clock_obj, structural_objectRef& reset_obj)
{
   const structural_managerRef& SM = this->HLS->datapath;
   const structural_objectRef& circuit = SM->get_circ();

   /// define boolean type for clock and reset signal
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * Start adding clock signal...");
   /// add clock port
   clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, port_type);
   GetPointer<port_o>(clock_obj)->set_is_clock(true);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    Clock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * Start adding reset signal...");
   /// add reset port
   reset_obj = SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    Reset signal added!");

   return;
}

void classic_datapath::add_ports()
{
   bool need_start_done = false;
   const structural_managerRef SM = this->HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();

   const std::list<unsigned int>& function_parameters = BH->get_parameters();
   for(auto const function_parameter : function_parameters)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Adding port for parameter: " + BH->PrintVariable(function_parameter) + " IN");
      generic_objRef port_obj;
      if(HLS->Rconn)
      {
         conn_binding::direction_type direction = conn_binding::IN;
         port_obj = HLS->Rconn->get_port(function_parameter, direction);
      }
      structural_type_descriptorRef port_type;
      if(HLSMgr->Rmem->has_base_address(function_parameter) && !HLSMgr->Rmem->has_parameter_base_address(function_parameter, HLS->functionId) && !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 32));
      }
      else
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor(function_parameter, BH));
      }
      if(HLSMgr->Rmem->has_base_address(function_parameter) && (HLSMgr->Rmem->is_parm_decl_stored(function_parameter) || HLSMgr->Rmem->is_parm_decl_copied(function_parameter)))
         need_start_done = true;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---type is: " + port_type->get_name());
      std::string prefix = "in_port_";
      port_o::port_direction port_direction = port_o::IN;
      structural_objectRef p_obj = SM->add_port(prefix + BH->PrintVariable(function_parameter), port_direction, circuit, port_type);
      if(HLS->Rconn)
      {
         port_obj->set_structural_obj(p_obj);
         port_obj->set_out_sign(p_obj);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added");
   }
   if(HLS->Rconn)
   {
      std::map<conn_binding::const_param, generic_objRef> const_objs = HLS->Rconn->get_constant_objs();
      unsigned int num = 0;
      for(auto& c : const_objs)
      {
         generic_objRef constant_obj = c.second;
         structural_objectRef const_obj = SM->add_module_from_technology_library("const_" + STR(num), CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_T->get_technology_manager());

         std::string value = std::get<0>(c.first);
         std::string param = std::get<1>(c.first);
         std::string trimmed_value;
         unsigned int precision;
         if(param.size() == 0)
         {
            trimmed_value = "\"" + std::get<0>(c.first) + "\"";
            precision = static_cast<unsigned int>(value.size());
         }
         else
         {
            trimmed_value = param;
            memory::add_memory_parameter(SM, param, std::get<0>(c.first));
            precision = GetPointer<dataport_obj>(constant_obj)->get_bitsize();
         }
         const_obj->SetParameter("value", trimmed_value);
         constant_obj->set_structural_obj(const_obj);
         std::string name = "out_const_" + std::to_string(num);
         structural_type_descriptorRef sign_type = structural_type_descriptorRef(new structural_type_descriptor("bool", precision));
         structural_objectRef sign = SM->add_sign(name, circuit, sign_type);
         structural_objectRef out_port = const_obj->find_member("out1", port_o_K, const_obj);
         // customize output port size
         out_port->type_resize(precision);
         SM->add_connection(sign, out_port);
         constant_obj->set_out_sign(sign);
         num++;
      }
   }
   const unsigned int return_type_index = BH->GetFunctionReturnType(BH->get_function_index());
   if(return_type_index)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Return type: " + BH->print_type(return_type_index));

      generic_objRef port_obj;
      if(HLS->Rconn)
      {
         port_obj = HLS->Rconn->get_port(return_type_index, conn_binding::OUT);
      }
      structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor(return_type_index, BH));
      structural_objectRef p_obj = SM->add_port(RETURN_PORT_NAME, port_o::OUT, circuit, port_type);
      if(HLS->Rconn)
      {
         port_obj->set_structural_obj(p_obj);
      }
   }
   /// add start and done when needed
   if(need_start_done)
   {
      structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      SM->add_port(START_PORT_NAME, port_o::IN, circuit, bool_type);
      SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, bool_type);
   }
}
