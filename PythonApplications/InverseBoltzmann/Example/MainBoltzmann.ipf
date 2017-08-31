// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#include "::Boltzmann"
#include "::::IgorCode:Util:PlotUtil"
#pragma ModuleName = ModMainBoltzmann

Static StrConstant DEF_INPUT_REL_TO_BASE =  "IgorUtil/PythonApplications/InverseBoltzmann/Example/Experiment.pxp"

Static Function Main_Windows()
	// Runs a simple IWT on patrick's windows setup
	ModMainBoltzmann#Main("C:/Users/pahe3165/src_prh/")
End Function 

Static Function Main_Mac()
	// Runs a simple IWT on patrick's mac setup 
	ModMainBoltzmann#Main("/Users/patrickheenan/src_prh/")
End Function

Static Function Main(base,[input_file])
	// // This function shows how to use the IWT code
	// Args:
	//		base: the folder where the Research Git repository lives 
	//		input_file: the pxp to load. If not present, defaults to 
	//		<base>DEF_INPUT_REL_TO_BASE
	String base,input_file
	if (ParamIsDefault(input_file))
		input_file  = base +DEF_INPUT_REL_TO_BASE
	EndIf
	KillWaves /A/Z
	ModPlotUtil#KillAllGraphs()
	// IWT options
	Struct BoltzmannOptions opt
	opt.number_of_bins = 20
	opt.interpolation_factor = 1
      opt.smart_interpolation = 1
      // Note: this stdev is from simulated data; 20nm would be
      // a huge (and awful) gaussian / point-spread function
      opt.gaussian_stdev  = 2e-8
      opt.output_interpolated = 1
      opt.n_iters = 300
	// add the file information
	opt.meta.path_to_input_file = input_file
	opt.meta.path_to_research_directory = base
	// Make the output waves
	Struct BoltzmannOutput output
	Make /O/N=0,  output.extension_bins,output.distribution,output.distribution_deconvolved
	// Execte the command
	ModBoltzmann#inverse_boltzmann(opt,output)
	// plot the distributions and energy landscapes, units of kT
	Variable kT =  1
	Make /O/N=(DimSize(output.distribution,0)) landscape,landscape_deconvolved
	landscape[] = -ln(output.distribution[p]) * kT
	landscape_deconvolved[] = -ln(output.distribution_deconvolved[p]) * kT
	ModPlotUtil#figure(hide=0)
	ModPlotUtil#subplot(2,1,1)
	ModPlotUtil#plot(output.distribution,mX=output.extension_bins)
	ModPlotUtil#plot(output.distribution_deconvolved,mX=output.extension_bins,color="r",linestyle="--")
	ModPlotUtil#pLegend(labelStr="Measured,Deconvolved")
	ModPlotUtil#xlabel("Extension (m)")
	ModPlotUtil#ylabel("Probability (1/m)")
	ModPlotUtil#subplot(2,1,2)
	ModPlotUtil#plot(landscape,mX=output.extension_bins)
	ModPlotUtil#plot(landscape_deconvolved,mX=output.extension_bins,color="r",linestyle="--")
	ModPlotUtil#xlabel("Extension (m)")
	ModPlotUtil#ylabel("Energy (kT)")


End Function
