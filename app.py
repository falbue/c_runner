from flask import Flask, render_template, request, jsonify
import docker
import tempfile
import os
import logging
from pathlib import Path

app = Flask(__name__)
app.logger.setLevel(logging.INFO)

# Инициализируем Docker клиент
try:
    docker_client = docker.from_env()
    docker_client.ping()
except Exception as e:
    app.logger.error(f"Docker connection error: {e}")
    docker_client = None

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/run', methods=['POST'])
def run_code():
    if not docker_client:
        return jsonify({'error': 'Docker service unavailable'}), 500

    code = request.json.get('code', '').strip()
    if not code:
        return jsonify({'error': 'Empty code'}), 400

    try:
        # Создаем временную директорию
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_file = Path(temp_dir) / 'user_code.c'
            
            # Записываем код в файл
            temp_file.write_text(code, encoding='utf-8')
            
            # Запускаем контейнер
            container = docker_client.containers.run(
                'c_runner',
                command=['sh', '-c', 'cd /app && ./runner < /tmp/code/user_code.c'],
                volumes={
                    str(temp_dir): {
                        'bind': '/tmp/code',
                        'mode': 'ro'
                    }
                },
                working_dir='/app',
                detach=True,
                stdout=True,
                stderr=True
            )
            
            # Ждем завершения и получаем вывод
            result = container.wait()
            output = container.logs(stdout=True, stderr=True).decode()
            
            # Удаляем контейнер после получения логов
            container.remove()
            
            return jsonify({
                'output': output,
                'exit_code': result['StatusCode']
            })
            
    except docker.errors.DockerException as e:
        app.logger.error(f"Docker error: {e}")
        return jsonify({'error': 'Execution error'}), 500
    except Exception as e:
        app.logger.error(f"Unexpected error: {e}")
        return jsonify({'error': 'Internal server error'}), 500

if __name__ == '__main__':
    app.run(debug=False, host='0.0.0.0', port=5000)