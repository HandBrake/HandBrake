// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewModelBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A class to relay actions from a view to view Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------
namespace HandBrakeWPF.Commands
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Reflection;
    using System.Windows.Input;

    using HandBrake.App.Core.Exceptions;

    public class RelayCommand : ICommand
    {
        private readonly Type type;

        private object viewModel;

        public RelayCommand(object viewModel)
        {
            this.viewModel = viewModel;
            type = viewModel.GetType();
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            string action = parameter as string;
            if (!string.IsNullOrEmpty(action))
            {
                string parameterData = string.Empty;
                if (action.Contains(","))
                {
                    parameterData = action.Split(',')[1];
                    action = action.Split(',')[0];
                }

                MethodInfo info = type.GetMethod(action);
                if (info != null)
                {
                    try
                    {
                        if (info.GetParameters().Length == 0)
                        {
                            info.Invoke(viewModel, null);
                        }
                        else
                        {
                            // Limited to 1 parameter as present. 
                            ParameterInfo[] parameters = info.GetParameters();
                            List<object> invokeParams = new List<object>();

                            ParameterInfo firstParam = parameters.FirstOrDefault();

                            if (firstParam?.ParameterType == typeof(int))
                            {
                                int keyValue;
                                if (int.TryParse(parameterData, out keyValue))
                                {
                                    invokeParams.Add(keyValue);
                                }
                            }

                            info.Invoke(viewModel, invokeParams.ToArray());
                        }
                    }
                    catch (Exception e)
                    {
                        if (e.InnerException != null && e.InnerException.GetType() == typeof(GeneralApplicationException))
                        {
                            throw e.InnerException;
                        }

                        throw e;
                    }
                }
            }
        }

        public event EventHandler CanExecuteChanged;
    }
}
