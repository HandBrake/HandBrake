This is the port of the current WinForms GUI over to WPF.
It is an incomplete work-in-progress and not currently in a usable state!

Goals:

- Move as much of the busness logic into the ApplicaitonServices library so it's reusable.
- Rebuild the UI in WPF using MVVM approach using Caliburn Micro 1.2 as the base and Castle Windsor for IoC/DI