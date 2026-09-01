from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    robot_name = LaunchConfiguration('robot_name')
    initial_state = LaunchConfiguration('initial_state')

    status_parameters = PathJoinSubstitution([
        FindPackageShare('robot_dog_day2_topics'),
        'config',
        'status_publisher.yaml',
    ])
    state_parameters = PathJoinSubstitution([
        FindPackageShare('robot_dog_day3_services'),
        'config',
        'state_server.yaml',
    ])
    parameter_parameters = PathJoinSubstitution([
        FindPackageShare('robot_dog_day6_parameters'),
        'config',
        'parameter_demo.yaml',
    ])

    return LaunchDescription([
        DeclareLaunchArgument('robot_name', default_value='robot_dog_launch'),
        DeclareLaunchArgument('initial_state', default_value='STANDING'),
        Node(
            package='robot_dog_day2_topics',
            executable='status_publisher',
            name='status_publisher',
            output='screen',
            parameters=[status_parameters, {'robot_name': robot_name}],
        ),
        Node(
            package='robot_dog_day2_topics',
            executable='status_subscriber',
            name='status_subscriber',
            output='screen',
        ),
        Node(
            package='robot_dog_day3_services',
            executable='state_server',
            name='state_server',
            output='screen',
            parameters=[state_parameters, {'initial_state': initial_state}],
        ),
        Node(
            package='robot_dog_day4_actions',
            executable='gait_action_server',
            name='gait_action_server',
            output='screen',
        ),
        Node(
            package='robot_dog_day6_parameters',
            executable='parameter_demo',
            name='parameter_demo',
            output='screen',
            parameters=[parameter_parameters],
        ),
    ])
