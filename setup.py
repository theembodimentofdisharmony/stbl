from setuptools import setup, find_packages

setup(
    name="stbdl",
    version="0.5.2",
    packages=find_packages(),
    include_package_data=True,
    package_data={
        "stbdl": ["*.exe", "*.png"],
    },
    exclude_package_data={
        "": [".idea/*", "*.cc", "*.obj", "*.res"],
    },
    author="enstarep",
    author_email="enstarep@rncyk.org",
    description="A simple taskbar decoration library",
    long_description=open("README.md", encoding="utf-8").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/theembodimentofdisharmony/stbdl",
    license="MIT",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Operating System :: Microsoft :: Windows"
    ],
    python_requires=">=3.0",
)
