Caliburn

Running one of the build commands (build-*.cmd) will execute the NAnt build script in a the chosen configuration.  

Throughout the solution you will see files with the extensions .silverlight.cs or .wpf.cs which indicate major platform differences.  
Elsewhere I have used various ammounts of conditional compilation as necessary.

Adapters for popular DI frameworks I have included are:
Caliburn.Castle
Caliburn.Spring
Caliburn.StructureMap
Caliburn.Unity
Caliburn.Ninject
Caliburn.Autofac
Caliburn.MEF (technically not a DI framework)

Using a DI container is not necessary, as Caliburn has a simple built-in container it uses by default.

If you are using Prism, have a look at Caliburn.Prism.
Unit tests for Caliburn's features can be found in Tests.Caliburn.

Please see the samples folder for examples of how to use the most prominent features of Caliburn.  
There are identical samples for both WPF and Silverlight.  You can also find some how to's and larger examples there.  

I hope this helps you in your development of applications for WPF and Silverlight.  Enjoy!