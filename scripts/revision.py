import subprocess

rev = (
    subprocess
        .check_output(['git', 'describe', '--always', '--broken', '--long'])
        .strip()
        .decode('utf-8')
)
print('-DREVISION=\\"{}\\"'.format(rev))
