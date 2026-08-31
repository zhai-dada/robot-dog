from setuptools import find_packages, setup

package_name = 'robot_dog_basics'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='zhai',
    maintainer_email='zhaipengwei0728@gmail.com',
    description='ROS2 Python基础节点示例',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'hello_world_node = robot_dog_basics.hello_world_node:main',
        ],
    },
)
