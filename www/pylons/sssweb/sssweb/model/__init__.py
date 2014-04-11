from sqlalchemy import schema, types
from sssweb.model.meta import Session, Base

class Prefix(Base):
        __tablename__ = 'prefix'
	id = schema.Column('id', types.Integer,	schema.Sequence('prefix_id_seq'), primary_key=True)
	prefix = schema.Column('prefix', types.Text())

class Users(object):
        __tablename__ = 'users'
	id = schema.Column('id', types.Integer, schema.Sequence('users_id_seq'), primary_key=True)
	name = schema.Column('name', types.Text())
        password = schema.Column('password', types.Text())
	real_name = schema.Column('real_name', types.Text())

class UsersPrefixAssociation(object):
	__tablename__ = 'user_prefix_access'
        user_id = schema.Column('user_id', types.Integer, schema.ForeignKey('users.id'), primary_key=True)
	prefix_id = schema.Column('prefix_id', types.Integer, schema.ForeignKey('prefix.id'), primary_key=True)
	write_access = schema.Column('write_access', types.Integer)

def init_model(engine):
	"""Call me before using any of the tables or classes in the model"""
	
        Session.configure(bind=engine)

