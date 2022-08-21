const fs = require('fs')

module.exports = async ({github, context, core}) => {
  const {SHA} = process.env
  const commit = await github.rest.repos.getCommit({
    owner: context.repo.owner,
    repo: context.repo.repo,
    ref: `${SHA}`
  })
  core.exportVariable('author', commit.data.commit.author.email)
  console.log(commit.data.commit.author.email)

  const dir = '.'
  const files = fs.readdirSync(dir)

  for (const file of files) {
    console.log(file)
  }
}
