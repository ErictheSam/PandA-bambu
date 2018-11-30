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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
 * @file DesignCompilerWrapper.hpp
 * @brief Wrapper to Design Compiler by Synopsys
 *
 * A object used to invoke Design Compiler by Synopsys for Logic Synthesis
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _DESIGN_COMPILER_WRAPPER_HPP_
#define _DESIGN_COMPILER_WRAPPER_HPP_

/// STD include
#include <string>

/// STL include
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

/// Autoheader includes
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_LOGIC_SYNTHESIS_FLOW_BUILT.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"

/// Utility include
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(ToolManager);
#if HAVE_TECHNOLOGY_BUILT
CONSTREF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
#endif
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(DesignParameters);
REF_FORWARD_DECL(xml_script_node_t);
REF_FORWARD_DECL(area_model);
REF_FORWARD_DECL(time_model);

#include <map>
#include <set>
#include <string>
#include <vector>

#include "SynopsysWrapper.hpp"

#define DESIGN_COMPILER_TOOL_ID std::string("dc_shell")

/**
 * @class DesignCompilerWrapper
 * Main class for wrapping Design Compiler by Synopsys
 */
class DesignCompilerWrapper : public SynopsysWrapper
{
 public:
   typedef enum
   {
      REPORT_AREA = 0,
      REPORT_TIME,
      REPORT_POWER,
      REPORT_CELL,
      SYNTHESIS_RESULT,
      SYNTHESIS_LOG,
      SDC_CONSTRAINTS
   } report_t;

   typedef enum
   {
      MEDIUM = 0,
      HIGH,
      ULTRA
   } opt_level_t;

   typedef enum
   {
      AREA = 0,
      TIME,
      POWER
   } constraint_t;

 protected:
   /**
    * Initializes the reserved vars
    */
   void init_reserved_vars() override;

   /// top module
   std::string top_module;

   /// constraint file
   std::string constraint_file;

   /// map between the type of the reports and the corresponding file generated by Design Compiler's execution
   std::map<unsigned int, std::string> report_files;

   /// path where the files have to be searched
   std::vector<std::string> search_path;

   /// libraries for linking
   std::vector<std::string> link_libs;

   /// target libraries for the synthesis process
   std::vector<std::string> target_libs;

   /// synthetic libraries for the synthesis process
   std::vector<std::string> synthetic_libs;

   /// list of libraries to be considered
   std::vector<std::string> library_list;

   /// max area
   double max_area;

   /// max area
   double max_delay;

   /// map between the name of the benchmark and the dont_use string to be set
   std::map<std::string, std::string> dont_use_map;

   /// area values coming out from the synthesis
   std::map<unsigned int, double> area;

   /// synthesis time
   double synthesis_time;
   /// results of the synthesis
   bool synthesis_result;

   /// arrival time of the different critical paths
   std::vector<double> arrival_time;

   /// map for the power values
   std::map<unsigned int, double> power;

   /// vector of critical paths
   std::vector<std::vector<std::string>> critical_paths;
   /// vector of cells in the critical paths
   std::vector<std::vector<std::string>> critical_cells;

   /// map between the name of the cell and the corresponding frequency
   std::map<std::string, unsigned int> cell_frequency;

   /**
    * Determine and load the link (i.e., initial) libraries into the synthesis script
    */
   void set_link_libraries(const DesignParametersRef dp);

   /**
    * Determine and load the target libraries into the synthesis script
    */
   void set_target_libraries(const DesignParametersRef dp);

   /**
    * Determine the synthesis constraints
    */
   void set_constraints(const DesignParametersRef dp);

   /**
    * Determine the search path
    */
   void set_search_path(const DesignParametersRef dp);

   /**
    * parse synthesis reports
    */
   void parse_synthesis_reports();

   /**
    * parse cell reports
    */
   void parse_cell_reports();

   /**
    * wrapper to parse all reports
    */
   void parse_reports();

 public:
   /**
    * parse area reports
    */
   area_modelRef parse_area_reports();

   /**
    * parse time reports
    */
   time_modelRef parse_time_reports();

   /**
    * Set a link library
    */
   void add_link_library(const std::string& link_library);

   /**
    * Set a vector of link libraries
    */
   void add_link_library(const std::vector<std::string>& link_library);

   /**
    * Set a target library
    */
   void add_target_library(const std::string& target_library);

   /**
    * Set a vector of target libraries
    */
   void add_target_library(const std::vector<std::string>& target_library);

   /**
    * Determine the set of cells to be used for each design
    */
   void add_dont_use_cells(const std::string& top_module, const std::string& dont_use_cells);

   /**
    * Set the search path
    */
   void set_search_path(const std::string& path);

   /**
    * Sets the top module
    */
   void set_top_module(const std::string& top);

   /**
    * Sets the path to the constraint file (SDC)
    */
   void set_constraint_file(const std::string& path);

 protected:
   /**
    * Write the timing path into an XML file
    */
   std::string write_timing_paths(const std::string& design_name, const std::vector<std::string>& timing_path);

   /**
    * Create the piece of script related of the initial design
    */
   std::string import_input_design(const DesignParametersRef dp, const std::vector<std::string>& file_list);

   /**
    * Write the reports about area, time and power
    */
   void write_reports(const DesignParametersRef dp);

   /**
    * Save the resulting netlist
    */
   void save_design(const DesignParametersRef dp, const std::string& target);

   /**
    * Perform the synthesis and optimization
    */
   void perform_optimization(const DesignParametersRef dp);

   /**
    * Evaluates variables
    */
   void EvaluateVariables(const DesignParametersRef dp) override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    * @param output_dir is the path to the directory where all the results will be stored
    */
   DesignCompilerWrapper(const ParameterConstRef _Param, const target_deviceRef _device, const std::string& _output_dir);

   /**
    * Destructor
    */
   ~DesignCompilerWrapper() override;

   /**
    * Return synthesis time
    */
   double get_synthesis_time() const;

   /**
    * Return area values
    */
   std::map<unsigned int, double> get_area_values() const;

   /**
    * Return arrival times
    */
   std::vector<double> get_arrival_time() const;

   /**
    * Return synthesis time
    */
   std::map<unsigned int, double> get_power_values() const;

   /**
    * Return critical cells
    */
   std::vector<std::vector<std::string>> get_critical_cells() const;

   /**
    * Return cell frequency
    */
   std::map<std::string, unsigned int> get_cell_frequency() const;

   /**
    * Return if the synthesis is completed without errors or not
    */
   bool get_synthesis_result() const;

   /**
    * Return a report file
    */
   std::string get_report_file(unsigned int report_type) const;
};
/// Refcount definition for the class
typedef refcount<DesignCompilerWrapper> DesignCompilerWrapperRef;

#endif
