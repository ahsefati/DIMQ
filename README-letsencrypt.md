# Using Lets Encrypt with dimq

On Unix like operating systems, dimq will attempt to drop root access as
soon as it has loaded its configuration file, but before it has activated any
of that configuration. This means that if you are using Lets Encrypt TLS
certificates, it will be unable to access the certificates and private keys
typically located in /etc/letsencrypt/live/

To help with this problem there is an example `deploy` renewal hook script in
`misc/letsencrypt/dimq-copy.sh` which shows how the certificate and
private key for a dimq broker can be copied to /etc/dimq/certs/ and
given the correct ownership and permissions so the broker can access them, but
no other user can. It then signals dimq to reload the certificates.

Use of this script allows you to happily use Lets Encrypt certificates with
dimq without needing root access for dimq, and without having to
restart dimq.
