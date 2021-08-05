#!/bin/bash

## This program attempts to automatically update the package-signing keys (from debian.krontech.ca) so that new signing keys can be used,

## Nicholas Faliszewski
## August 2021


## first, get a list of the available keys
list=$(wget http://debian.krontech.ca/krontech-archive.gpg -qO- | gpg --keyid-format long --dry-run | grep "Krontech Package Signing Key")

echo "list was: $list"


while read line ## run through each line that's named "Krontech Package Signing Key..."
do
	a=${line#*"4096R/"} ## find the next key ID
	a=${a%%" "*} ## make it so that we just have the key ID (remove everything after (and including) the space))
	echo "found key ID: $a"

	apt-key adv --keyserver http://debian.krontech.ca/krontech-archive.gpg --recv-keys $a ## add each key (it will be ignored if alreay imported)
	
done <<< "$list" ## run through each line



## gpg: key D387E8DA: public key "Krontech Package Signing Key <it@krontech.ca>" ## this was Sanjay's key
## gpg: key DCE92114: "Krontech Package Signing Key (nf) (Signing key made by Nicholas) <software@krontech.ca>" ## my key
## gpg: key C43184EA: "Krontech Package Signing Key <software@krontech.ca>" ## old key

## Check a password for a key: echo "1234" | gpg -o /dev/null --local-user D06917EB58347EEE6EF50443FD9367E9DCE92114 -as - && echo "The correct passphrase was entered for this key"