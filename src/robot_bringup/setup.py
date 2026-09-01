from setuptools import setup

package_name = 'robot_bringup'

setup(
    name=package_name,
    version='0.0.0',
    packages=[],
    data_files=[
        ('share/ament_index/resource_index/packages',
         ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', ['launch/robot_dog_demo.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='zhai',
    maintainer_email='zhaipengwei0728@gmail.com',
    description='robot-dog系统的Python Launch启动入口',
    license='Apache-2.0',
)
