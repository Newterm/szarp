<%inherit file="base.mako"/>

${h.form(h.url_for(method='post'))}
<p>${_('Current password')}: ${h.password('password')}</p>
<p>${_('New password')}: ${h.password('new_password')}</p>
<p>${_('Re-enter new password')}: ${h.password('new_password2')}</p>
${h.submit('Submit', _('Submit'))}
${h.end_form()}

