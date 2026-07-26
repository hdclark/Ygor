#!/usr/bin/env bash
set -euo pipefail
git config user.name "OpenAI"
git config user.email "noreply@openai.com"
cat .github/component07-patch-b64/part-* > /tmp/component07-base.patch.gz.b64
test "$(sha256sum /tmp/component07-base.patch.gz.b64 | cut -d' ' -f1)" = "0b8964f71fd3fdf14411a2d22055e8e387d3eaeb5794b399d1547b7ed7948a43"
base64 --decode /tmp/component07-base.patch.gz.b64 | gzip --decompress > /tmp/component07-base.patch
test "$(sha256sum /tmp/component07-base.patch | cut -d' ' -f1)" = "50af3dee3845e9c1e18e9bd1d9928caa0f93869864de94af4bd9c51d768f9611"
git am --3way /tmp/component07-base.patch
cat .github/component07-family04-b64/part-* > /tmp/component07-family04.patch.gz.b64
test "$(sha256sum /tmp/component07-family04.patch.gz.b64 | cut -d' ' -f1)" = "7d80f49686d25a60d0efd089612cab92d05046e49923130f53b8ea7a71cd0e8e"
base64 --decode /tmp/component07-family04.patch.gz.b64 | gzip --decompress > /tmp/component07-family04.patch
test "$(sha256sum /tmp/component07-family04.patch | cut -d' ' -f1)" = "08e46e21d7b8a7b2fd8d6a84c2420ff29ab7750e685faaf85937d0399985b717"
git am --3way /tmp/component07-family04.patch
git rm -r .github/component07-patch-b64 .github/component07-family04-b64 .github/component07-series.ready .github/component07-apply.sh
printf '%s' 'bmFtZTogQ29tcG9uZW50IDA3IHdvcmtzcGFjZSBleHBvcnQKCm9uOgogIHB1bGxfcmVxdWVzdDoKICAgIGJyYW5jaGVzOgogICAgICAtIG1hc3RlcgogICAgICAtIGFnZW50L2NvbXBvbmVudDA3LXJ1bm5lci1iYXNlCiAgcHVzaDoKICAgIGJyYW5jaGVzOgogICAgICAtIGFnZW50L2JvdW5kZWQtbWFuaWZvbGQtbWVzaC1ib29sZWFuLXBsYW4KCmpvYnM6CiAgZXhwb3J0OgogICAgcnVucy1vbjogdWJ1bnR1LWxhdGVzdAogICAgdGltZW91dC1taW51dGVzOiAxNQogICAgc3RlcHM6CiAgICAgIC0gdXNlczogYWN0aW9ucy9jaGVja291dEB2NAogICAgICAgIHdpdGg6CiAgICAgICAgICBmZXRjaC1kZXB0aDogMAogICAgICAtIG5hbWU6IFBhY2thZ2UgY2hlY2tvdXQKICAgICAgICBydW46IHwKICAgICAgICAgIGdpdCBzdGF0dXMgLS1zaG9ydCAtLWJyYW5jaAogICAgICAgICAgdGFyIC0tZXhjbHVkZT0uZ2l0IC0tZXhjbHVkZT1idWlsZCAtY3pmIC90bXAveWdvci1jb21wb25lbnQwNy13b3Jrc3BhY2UudGFyLmd6IC4KICAgICAgLSB1c2VzOiBhY3Rpb25zL3VwbG9hZC1hcnRpZmFjdEB2NAogICAgICAgIHdpdGg6CiAgICAgICAgICBuYW1lOiB5Z29yLWNvbXBvbmVudDA3LXdvcmtzcGFjZQogICAgICAgICAgcGF0aDogL3RtcC95Z29yLWNvbXBvbmVudDA3LXdvcmtzcGFjZS50YXIuZ3oKICAgICAgICAgIGlmLW5vLWZpbGVzLWZvdW5kOiBlcnJvcgogICAgICAgICAgcmV0ZW50aW9uLWRheXM6IDEK' | base64 --decode > .github/workflows/component07-workspace.yml
git add .github/workflows/component07-workspace.yml
git commit -m "Component 07: remove applied implementation bundle"
git push origin HEAD:agent/bounded-manifold-mesh-boolean-plan
