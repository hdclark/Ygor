#!/usr/bin/env bash
set -euo pipefail
git config user.name "OpenAI"
git config user.email "noreply@openai.com"
cat .github/component07-patch-b64/part-* > /tmp/component07-base.patch.gz.b64
test "$(sha256sum /tmp/component07-base.patch.gz.b64 | cut -d' ' -f1)" = "0b8964f71fd3fdf14411a2d22055e8e387d3eaeb5794b399d1547b7ed7948a43"
base64 --decode /tmp/component07-base.patch.gz.b64 | gzip --decompress > /tmp/component07-base.patch
test "$(sha256sum /tmp/component07-base.patch | cut -d' ' -f1)" = "50af3dee3845e9c1e18e9bd1d9928caa0f93869864de94af4bd9c51d768f9611"
rm -rf /tmp/component07-base-mails
mkdir /tmp/component07-base-mails
test "$(git mailsplit -o/tmp/component07-base-mails /tmp/component07-base.patch)" = "2"
git am --3way /tmp/component07-base-mails/0002
cat .github/component07-family04-b64/part-* > /tmp/component07-family04.patch.gz.b64
test "$(sha256sum /tmp/component07-family04.patch.gz.b64 | cut -d' ' -f1)" = "7d80f49686d25a60d0efd089612cab92d05046e49923130f53b8ea7a71cd0e8e"
base64 --decode /tmp/component07-family04.patch.gz.b64 | gzip --decompress > /tmp/component07-family04.patch
test "$(sha256sum /tmp/component07-family04.patch | cut -d' ' -f1)" = "08e46e21d7b8a7b2fd8d6a84c2420ff29ab7750e685faaf85937d0399985b717"
git am --3way /tmp/component07-family04.patch
git rm -r .github/component07-patch-b64 .github/component07-family04-b64 .github/component07-series.ready .github/component07-apply.sh
git commit -m "Component 07: remove applied implementation bundle"
git push origin HEAD:agent/bounded-manifold-mesh-boolean-plan
