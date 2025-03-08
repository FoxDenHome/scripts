#!/bin/bash
set -euo pipefail

if [ ! -f ~/.ssh/id_ed25519.github-foxdenhome ]; then
    echo 'Generating SSH key for GitHub...'
    ssh-keygen -q -N '' -t ed25519 -f ~/.ssh/id_ed25519.github-foxdenhome
else
    echo 'SSH key for GitHub already exists.'
fi

if ! grep -qF github.com ~/.ssh/config; then
    echo 'Adding GitHub SSH config...'
    echo 'Host github.com' >> ~/.ssh/config
    echo '  IdentityFile ~/.ssh/id_ed25519.github-foxdenhome' >> ~/.ssh/config
else
    echo 'GitHub SSH config already exists.'
fi

git config --global url.ssh://git@github.com/.insteadOf https://github.com/

echo 'Add the following key to FoxDenServers GitHub:'
cat ~/.ssh/id_ed25519.github-foxdenhome.pub
echo 'Link: https://github.com/settings/keys'

echo 'Will run ssh to confirm key is working and trust host keys'
read -p 'Press key to continue... ' -n1 -s
echo 'OK!'
ssh git@github.com
