@echo off

set OPENSSL=%PRODUCT_ROOT%\unsupported\openssl\ix86_win32\bin\openssl.exe
set CERT_NAME=example_certstore

%OPENSSL% genrsa -out %CERT_NAME%_privkey.pem 2048

%OPENSSL% rsa -in %CERT_NAME%_privkey.pem -out %CERT_NAME%_pubkey.pem -pubout

%OPENSSL% req -config config_openssl.txt -new -key %CERT_NAME%_privkey.pem -out %CERT_NAME%_csr.pem
%OPENSSL% req -config config_openssl.txt -in %CERT_NAME%_csr.pem -text

%OPENSSL% x509 -req -sha1 -in %CERT_NAME%_csr.pem -signkey %CERT_NAME%_privkey.pem -CAcreateserial -out %CERT_NAME%.crt

%OPENSSL% x509 -in %CERT_NAME%.crt -outform DER -out %CERT_NAME%.der
%OPENSSL% x509 -in %CERT_NAME%.crt -text
