param($installPath, $toolsPath, $package, $project)
Write-Host "Adding Behaviors SDK (XAML)"
$project.Object.References.AddSDK("Behaviors SDK (XAML)", "BehaviorsXamlSDKManaged, version=12.0")