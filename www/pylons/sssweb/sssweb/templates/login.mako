<%inherit file="base.mako"/>
  
<div class="login">
${h.form(h.url_for(action='submit', method='post'))}
<p>${_('Login')}: ${h.text('username')}</p>
<p>${_('Password')}: ${h.password('password')}</p>
<p>${h.link_to(_('Forgot password?'), h.url_for(action="reminder"))}</p>
${h.submit('login', _('Log in'))}
${h.end_form()}
</div>


