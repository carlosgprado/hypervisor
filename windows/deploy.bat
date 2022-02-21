@echo off


signtool.exe sign /v /s PrivateCertStore /n WeLikeDrivers(Test) /t http://timestamp.digicert.com c:\Users\user\Software\hypervisor\windows\objchk_win7_amd64\amd64\HypervisorDriver.sys

sc create HypervisorDriver type= kernel binPath=c:\Users\user\Software\hypervisor\windows\objchk_win7_amd64\amd64\HelloWorldDriver.sys

