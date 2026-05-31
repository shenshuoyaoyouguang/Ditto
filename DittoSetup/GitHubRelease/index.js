const axios = require('axios');
const fs = require('fs/promises');
const { env } = require('process');

const repo = env.GITHUB_REPOSITORY || 'sabrogden/Ditto';
const apiBase = `https://api.github.com/repos/${repo}`;
const uploadBase = `https://uploads.github.com/repos/${repo}`;
const headers = {
    'X-GitHub-Api-Version': '2022-11-28',
    'Authorization': `token ${env.token}`
};

async function FindReleaseByTag(tag) {
    try {
        const config = { method: 'get', headers, url: `${apiBase}/releases/tags/${tag}` };
        const res = await axios(config);
        return res.data;
    } catch (e) {
        if (e.response && e.response.status === 404) return null;
        throw e;
    }
}

async function CreateRelease(tag) {
    const config = {
        method: 'post',
        headers,
        url: `${apiBase}/releases`,
        data: {
            tag_name: tag,
            name: tag,
            body: '',
            draft: false,
            prerelease: false
        }
    };
    console.log(`Creating release for tag: ${tag}`);
    const res = await axios(config);
    return res.data;
}

async function DeleteOldAssets(release) {
    for (const asset of release.assets) {
        const config = {
            method: 'DELETE',
            headers,
            url: `${apiBase}/releases/assets/${asset.id}`
        };
        console.log(`Deleting file ${asset.name}, id: ${asset.id}`);
        await axios(config);
    }
}

async function UploadFiles(release) {
    let files;
    try {
        files = await fs.readdir(env.uploadPath);
    } catch (e) {
        console.log(`Upload path ${env.uploadPath} does not exist or is empty`);
        return;
    }

    for (const file of files) {
        if (file.startsWith('Ditto')) {
            const fullPath = `${env.uploadPath}${file}`;
            const fileBytes = await fs.readFile(fullPath);

            const config = {
                method: 'POST',
                headers: {
                    ...headers,
                    'Content-Type': 'application/octet-stream',
                    'Content-Length': fileBytes.length
                },
                url: `${uploadBase}/releases/${release.id}/assets?name=${file}`,
                data: fileBytes
            };

            console.log(`Uploading file ${fullPath}`);
            await axios(config);
        }
    }
}

async function UpdateReleaseNotes(release) {
    let releaseNotes = '';

    try {
        const config = {
            method: 'POST',
            headers,
            url: `${apiBase}/releases/generate-notes`,
            data: {
                tag_name: env.tag,
                previous_tag_name: env.previous_tag
            }
        };

        console.log(`Generating release notes for tag: ${env.tag}, previous: ${env.previous_tag}`);
        const res = await axios(config);
        releaseNotes = res.data.body;
    } catch (e) {
        console.log(`Could not generate release notes: ${e.message}`);
        releaseNotes = `Release ${env.tag}`;
    }

    const config = {
        method: 'PATCH',
        headers,
        url: `${apiBase}/releases/${release.id}`,
        data: { body: releaseNotes }
    };

    console.log(`Updating release notes`);
    await axios(config);
}

async function Run() {
    const tag = env.tag;
    if (!tag) {
        console.error('env.tag is not set');
        process.exit(1);
    }

    let release = await FindReleaseByTag(tag);

    if (release) {
        console.log(`Found existing release for tag: ${tag}`);
        await DeleteOldAssets(release);
    } else {
        release = await CreateRelease(tag);
    }

    await UploadFiles(release);
    await UpdateReleaseNotes(release);

    console.log(`Release published: ${release.html_url}`);
}

Run().catch(e => {
    console.error(`Release failed: ${e.message}`);
    if (e.response) {
        console.error(`Status: ${e.response.status}`);
        console.error(`Data: ${JSON.stringify(e.response.data)}`);
    }
    process.exit(1);
});
