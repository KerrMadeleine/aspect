/*
  Copyright (C) 2011 - 2023 by the authors of the ASPECT code.

  This file is part of ASPECT.

  ASPECT is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  ASPECT is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ASPECT; see the file LICENSE.  If not see
  <http://www.gnu.org/licenses/>.
*/


#include <aspect/material_model/equation_of_state/multicomponent_incompressible.h>
#include <aspect/adiabatic_conditions/interface.h>
#include <aspect/utilities.h>


namespace aspect
{
  namespace MaterialModel
  {
    namespace EquationOfState
    {
      template <int dim>
      void
      MulticomponentIncompressible<dim>::
      evaluate(const MaterialModel::MaterialModelInputs<dim> &in,
               const unsigned int input_index,
               MaterialModel::EquationOfStateOutputs<dim> &eos_outputs) const
      {
        // If adiabatic heating is used, the reference temperature used to calculate density should be the adiabatic
        // temperature at the current position. This definition is consistent with the Extended Boussinesq Approximation.
        const double reference_temperature = (this->include_adiabatic_heating()
                                              ?
                                              this->get_adiabatic_conditions().temperature(in.position[input_index])
                                              :
                                              reference_T);

        for (unsigned int c=0; c < eos_outputs.densities.size(); ++c)
          {
            eos_outputs.densities[c] = densities[c] * (1 - thermal_expansivities[c] * (in.temperature[input_index] - reference_temperature));
            eos_outputs.thermal_expansion_coefficients[c] = thermal_expansivities[c];
            eos_outputs.specific_heat_capacities[c] = specific_heats[c];
            eos_outputs.compressibilities[c] = 0.0;
            eos_outputs.entropy_derivative_pressure[c] = 0.0;
            eos_outputs.entropy_derivative_temperature[c] = 0.0;
          }
      }



      template <int dim>
      bool
      MulticomponentIncompressible<dim>::
      is_compressible () const
      {
        return false;
      }



      template <int dim>
      void
      MulticomponentIncompressible<dim>::declare_parameters (ParameterHandler &prm,
                                                             const double default_thermal_expansion)
      {
        prm.declare_entry ("Reference temperature", "293.",
                           Patterns::Double (0.),
                           "The reference temperature $T_0$. Units: \\si{\\kelvin}.");
        prm.declare_entry ("Densities", "3300.",
                           Patterns::Anything(),
                           "List of densities for background mantle and compositional fields,"
                           "for a total of N+M+1 values, where N is the number of compositional fields and M is the number of phases. "
                           "If only one value is given, then all use the same value. "
                           "Units: \\si{\\kilogram\\per\\meter\\cubed}.");
        prm.declare_entry ("Thermal expansivities", std::to_string(default_thermal_expansion),
                           Patterns::Anything(),
                           "List of thermal expansivities for background mantle and compositional fields,"
                           "for a total of N+M+1 values, where N is the number of compositional fields and M is the number of phases. "
                           "If only one value is given, then all use the same value. Units: \\si{\\per\\kelvin}.");
        prm.declare_entry ("Heat capacities", "1250.",
                           Patterns::Anything(),
                           "List of specific heats $C_p$ for background mantle and compositional fields,"
                           "for a total of N+M+1 values, where N is the number of compositional fields and M is the number of phases. "
                           "If only one value is given, then all use the same value. "
                           "Units: \\si{\\joule\\per\\kelvin\\per\\kilogram}.");
        prm.declare_alias ("Heat capacities", "Specific heats");
      }



      template <int dim>
      void
      MulticomponentIncompressible<dim>::parse_parameters (ParameterHandler &prm,
                                                           const std::unique_ptr<std::vector<unsigned int>> &expected_n_phases_per_composition)
      {
        reference_T = prm.get_double ("Reference temperature");

        // Make options file for parsing maps to double arrays
        std::vector<std::string> chemical_field_names = this->introspection().chemical_composition_field_names();
        chemical_field_names.insert(chemical_field_names.begin(),"background");

        std::vector<std::string> compositional_field_names = this->introspection().get_composition_names();
        compositional_field_names.insert(compositional_field_names.begin(),"background");

        Utilities::MapParsing::Options options(chemical_field_names, "Densities");
        options.list_of_allowed_keys = compositional_field_names;
        options.allow_multiple_values_per_key = true;
        if (expected_n_phases_per_composition)
          {
            options.n_values_per_key = *expected_n_phases_per_composition;

            // check_values_per_key is required to be true to duplicate single values
            // if they are to be used for all phases associated with a given key.
            options.check_values_per_key = true;
          }
        else
          {
            // If the material model does not tell us how many phases per composition to expect,
            // at least check that the parameters parsed below have the same number of values.
            options.store_values_per_key = true;
          }

        // Parse multicomponent properties
        densities = Utilities::MapParsing::parse_map_to_double_array(prm.get("Densities"), options);

        // Now that we know we have stored the number of phases per composition, we can check them for subsequent properties.
        options.store_values_per_key = false;
        options.check_values_per_key = true;

        options.property_name = "Thermal expansivities";
        thermal_expansivities = Utilities::MapParsing::parse_map_to_double_array(prm.get("Thermal expansivities"), options);
        options.property_name = "Heat capacities";
        specific_heats = Utilities::MapParsing::parse_map_to_double_array (prm.get("Heat capacities"), options);
      }
    }
  }
}


// explicit instantiations
namespace aspect
{
  namespace MaterialModel
  {
    namespace EquationOfState
    {
#define INSTANTIATE(dim) \
  template class MulticomponentIncompressible<dim>;

      ASPECT_INSTANTIATE(INSTANTIATE)

#undef INSTANTIATE
    }
  }
}
