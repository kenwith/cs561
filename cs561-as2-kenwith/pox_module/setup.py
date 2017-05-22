#!/usr/bin/env python
'''Setuptools params'''

from setuptools import setup, find_packages

setup(
    name='cs561',
    version='0.0.0',
    description='Network controller for UW CS561 based on Stanford CS144 Lab',
    author='CS561 TA',
    author_email='mgliu@cs.washington.edu',
    url='https://courses.cs.washington.edu/courses/csep561/17sp/',
    packages=find_packages(exclude='test'),
    long_description="""\
Insert longer description here.
      """,
      classifiers=[
          "License :: OSI Approved :: GNU General Public License (GPL)",
          "Programming Language :: Python",
          "Development Status :: 1 - Planning",
          "Intended Audience :: Developers",
          "Topic :: Internet",
      ],
      keywords='uw cs561',
      license='GPL',
      install_requires=[
        'setuptools',
        'twisted',
        'ltprotocol', # David Underhill's nice Length-Type protocol handler
      ])

