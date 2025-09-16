from flask import Flask, render_template, request, redirect, url_for, flash, session

app = Flask(__name__)
app.secret_key = 'your_secret_key_here'

CORRECT_USERNAME = "intern123"
CORRECT_PASSWORD = "weakpassword456"
FLAG = "flag{http_p4r4m3t3r_p0llut10n_1s_fun!}"

@app.route('/')
def index():
    return render_template('login.html')

@app.route('/login', methods=['POST'])
def login():
    username = request.form.get('username')
    password = request.form.get('password')
    
    if username == CORRECT_USERNAME and password == CORRECT_PASSWORD:
        session['logged_in'] = True
        return redirect(url_for('mission', admin='False'))
    else:
        flash('Invalid credentials!')
        return redirect(url_for('index'))

@app.route('/mission')
def mission():
    if not session.get('logged_in'):
        return redirect(url_for('index'))
    
    admin_params = request.args.getlist('admin')
    is_admin = False
    
    # Vulnerable check - if any parameter is True, grant admin access
    if 'True' in admin_params and len(admin_params) > 1:
        is_admin = True
    
    return render_template('mission.html', is_admin=is_admin)

@app.route('/get_flag')
def get_flag():
    admin_params = request.args.getlist('admin')
    if 'True' in admin_params and len(admin_params) > 1:
        return {'flag': FLAG}
    return {'error': 'Nice try! But you need to be an admin!'}

if __name__ == '__main__':
    app.run(debug=True)
