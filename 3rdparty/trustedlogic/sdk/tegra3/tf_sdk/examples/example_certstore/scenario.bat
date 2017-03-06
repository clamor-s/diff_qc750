@echo off

IF "%1"=="-debug" goto client_debug


echo ------------------------------------
echo Certstore client in RELEASE mode
echo ------------------------------------
set PATH_TO_CLIENT=.\build\ix86_win32\Release

goto start_client


:client_debug

echo ------------------------------------
echo Certstore client in DEBUG mode
echo ------------------------------------
set PATH_TO_CLIENT=.\build\ix86_win32\Debug



:start_client

rem # Certificate at DER format	#
set CERT_VERISIGN_DER=BuiltinObjectToken-VerisignClass3PublicPrimaryCertificationAuthority.der

rem # Certificate at CRT format (not supported format) #
set CERT_VERISIGN_CRT=BuiltinObjectToken-VerisignClass3PublicPrimaryCertificationAuthority.crt



rem # Path to certstore_client.exe #
cd %PATH_TO_CLIENT%


echo ------------------------------------
echo Certstore_client help 		
echo ------------------------------------

certstore_client help



echo ------------------------------------
echo List of certstore_client commands 		
echo ------------------------------------

certstore_client installCert .\..\..\..\data\example_certstore.der
certstore_client installCert .\..\..\..\data\%CERT_VERISIGN_DER%

rem # Certificate with Bad Format #
certstore_client installCert .\..\..\..\data\%CERT_VERISIGN_CRT%


certstore_client listAll

certstore_client readCertDN 0x00000000

certstore_client deleteCert 0x000000000
certstore_client listAll

certstore_client getCert 0x00000001

certstore_client deleteAll
certstore_client listAll


rem # Path to scenario.bat #		
cd .\..\..\..