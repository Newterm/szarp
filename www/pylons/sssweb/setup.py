try:
    from setuptools import setup, find_packages
except ImportError:
    from ez_setup import use_setuptools
    use_setuptools()
    from setuptools import setup, find_packages

setup(
    name='sssweb',
    version="3.0.0",
    description='Web interface for administration of SZARP Synchronization Server',
    author='Pawel Palucha',
    author_email='pawel@praterm.com.pl',
    #url='',
    install_requires=["Pylons>=0.9.7"],
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
