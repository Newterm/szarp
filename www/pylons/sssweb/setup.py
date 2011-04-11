import os
import re

DEFAULT_VERSION='3.0.0'

try:
    from setuptools import setup, find_packages
except ImportError:
    from ez_setup import use_setuptools
    use_setuptools()
    from setuptools import setup, find_packages

def get_version():
    ver = DEFAULT_VERSION
    changelog = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../debian/changelog'))
    for line in changelog:
        print line
        m = re.search('^szarp\s\(([^)]*)\)', line)
        if m is not None:
            ver = m.group(1)
            break
    changelog.close()
    return ver

print get_version()

setup(
    name='sssweb',
    version=get_version(),
    description='Web interface for administration of SZARP Synchronization Server',
    author='Pawel Palucha',
    author_email='pawel@praterm.com.pl',
    #url='',
    install_requires=["Pylons>=0.10.0"],
    packages=find_packages(exclude=['ez_setup']),
    include_package_data=True,
    test_suite='nose.collector',
    package_data={'sssweb': ['i18n/*/LC_MESSAGES/*.mo']},
    message_extractors = {'sssweb': [
            ('**.py', 'python', None),
            ('templates/**.mako', 'mako', None),
            ('public/**', 'ignore', None)]},
    paster_plugins=['PasteScript', 'Pylons'],
    entry_points="""
    [paste.app_factory]
    main = sssweb.config.middleware:make_app

    [paste.app_install]
    main = pylons.util:PylonsInstaller
    """,
)
