import React, { useState, useEffect, useCallback, useImperativeHandle, forwardRef } from 'react';
import { Search, Plus, Users, MessageCircle, UserPlus, Settings, Trash2, LogOut } from 'lucide-react';
import { toast } from 'react-hot-toast';
import { useAuthStore } from '../stores/authStore';
import Modal from './Modal';
import GroupInvitations from './GroupInvitations';

interface Chat {
  id: string;
  name: string;
  lastMessage: string;
  timestamp: string;
  unreadCount: number;
  isGroup: boolean;
  otherUserId?: number;
  groupId?: number;
}

interface SidebarProps {
  onChatSelect: (chatId: string) => void;
  selectedChat: string | null;
}

const Sidebar = forwardRef<unknown, SidebarProps>(({ onChatSelect, selectedChat }, ref) => {
  const { token } = useAuthStore();
  const [searchQuery, setSearchQuery] = useState('');
  const [activeTab, setActiveTab] = useState<'chats' | 'groups'>('chats');
  const [chats, setChats] = useState<Chat[]>([]);
  const [groups, setGroups] = useState<Chat[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [isGroupModalOpen, setGroupModalOpen] = useState(false);
  const [isNewChatModalOpen, setNewChatModalOpen] = useState(false);
  const [isGroupInfoModalOpen, setGroupInfoModalOpen] = useState(false);
  const [groupName, setGroupName] = useState('');
  const [groupDescription, setGroupDescription] = useState('');
  const [isCreatingGroup, setIsCreatingGroup] = useState(false);
  const [newChatUser, setNewChatUser] = useState('');
  const [isStartingChat, setIsStartingChat] = useState(false);
  const [selectedGroupInfo, setSelectedGroupInfo] = useState<any>(null);
  const [groupMembers, setGroupMembers] = useState<any[]>([]);
  const [inviteUsername, setInviteUsername] = useState('');
  const [isInvitingUser, setIsInvitingUser] = useState(false);
  const [searchResults, setSearchResults] = useState<any[]>([]);
  const [isSearching, setIsSearching] = useState(false);

  const fetchChatSessions = useCallback(async () => {
    try {
      const response = await fetch('/chat-sessions', {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const chatSessions = data.data.map((session: any) => ({
          id: `chat-${session.id}`,
          name: session.other_user_name || 'Unknown User',
          lastMessage: session.last_message || 'No messages yet',
          timestamp: new Date(session.updated_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          unreadCount: session.unread_count || 0,
          isGroup: false,
          otherUserId: session.other_user_id
        }));
        setChats(chatSessions);
      } else {
        toast.error('Failed to load chat sessions');
      }
    } catch (error) {
      console.error('Failed to fetch chat sessions:', error);
      toast.error('Failed to load chat sessions');
    }
  }, [token]);

  const fetchGroups = useCallback(async () => {
    try {
      const response = await fetch('/api/groups', {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const groupChats = data.data.map((group: any) => ({
          id: `group-${group.id}`,
          name: group.name,
          lastMessage: 'Group chat',
          timestamp: new Date(group.created_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          unreadCount: 0,
          isGroup: true,
          groupId: group.id
        }));
        setGroups(groupChats);
      } else {
        console.error('Failed to load groups:', response.status, response.statusText);
        toast.error('Failed to load groups');
      }
    } catch (error) {
      console.error('Failed to fetch groups:', error);
      toast.error('Failed to load groups');
    }
  }, [token]);

  // Fetch chat sessions and groups
  useEffect(() => {
    const loadData = async () => {
      setIsLoading(true);
      try {
        await Promise.all([fetchChatSessions(), fetchGroups()]);
      } catch (error) {
        console.error('Failed to load data:', error);
      } finally {
        setIsLoading(false);
      }
    };
    
    loadData();
  }, [fetchChatSessions, fetchGroups]);

  const handleCreateGroup = async () => {
    setGroupModalOpen(true);
  };

  const submitCreateGroup = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!groupName.trim()) return;
    setIsCreatingGroup(true);
    try {
      const response = await fetch('/api/groups', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({ name: groupName, description: groupDescription })
      });
      if (response.ok) {
        toast.success('Group created successfully');
        await fetchGroups(); // Refresh groups list
        setGroupModalOpen(false);
        setGroupName('');
        setGroupDescription('');
      } else {
        toast.error('Failed to create group');
      }
    } catch (error) {
      console.error('Failed to create group:', error);
      toast.error('Failed to create group');
    } finally {
      setIsCreatingGroup(false);
    }
  };

  const handleNewConversation = () => {
    setNewChatModalOpen(true);
  };

  const handleUserSearch = async (searchTerm: string) => {
    console.log('handleUserSearch called with:', searchTerm);
    if (!searchTerm.trim()) {
      console.log('Empty search term, clearing results');
      setSearchResults([]);
      return;
    }
    
    setIsSearching(true);
    try {
      console.log('Making search request to:', `/api/users/search/${encodeURIComponent(searchTerm)}`);
      const response = await fetch(`/api/users/search/${encodeURIComponent(searchTerm)}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      console.log('Search response status:', response.status);
      if (response.ok) {
        const data = await response.json();
        console.log('Search response data:', data);
        setSearchResults(data.data || []);
      } else {
        console.error('Failed to search users:', response.status);
        setSearchResults([]);
      }
    } catch (error) {
      console.error('Failed to search users:', error);
      setSearchResults([]);
    } finally {
      setIsSearching(false);
    }
  };

  const handleStartConversation = async (userId: number, username: string) => {
    try {
      const response = await fetch('/api/chat-sessions', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({ other_user_id: userId })
      });
      
      if (response.ok) {
        toast.success(`Started conversation with ${username}`);
        setNewChatModalOpen(false);
        setNewChatUser('');
        setSearchResults([]);
        await fetchChatSessions(); // Refresh chat sessions
      } else {
        const error = await response.json();
        toast.error(error.message || 'Failed to start conversation');
      }
    } catch (error) {
      console.error('Failed to start conversation:', error);
      toast.error('Failed to start conversation');
    }
  };

  const handleGroupClick = async (group: Chat) => {
    onChatSelect(group.id);
  };

  const handleInviteUser = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!inviteUsername.trim()) return;
    setIsInvitingUser(true);
    try {
      const groupId = selectedGroupInfo.id;
      const response = await fetch(`/api/groups/${groupId}/members`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({ username: inviteUsername })
      });
      
      if (response.ok) {
        toast.success(`Invited ${inviteUsername} to the group`);
        setInviteUsername('');
        
        // Refresh group members
        const membersResponse = await fetch(`/api/groups/${groupId}/members`, {
          headers: {
            'Authorization': `Bearer ${token}`
          }
        });
        
        if (membersResponse.ok) {
          const membersData = await membersResponse.json();
          setGroupMembers(membersData.data || []);
        }
      } else {
        const error = await response.json();
        toast.error(error.message || 'Failed to invite user');
      }
    } catch (error) {
      console.error('Failed to invite user:', error);
      toast.error('Failed to invite user');
    } finally {
      setIsInvitingUser(false);
    }
  };

  const handleRemoveMember = async (userId: number) => {
    if (!confirm('Are you sure you want to remove this member?')) return;
    
    try {
      const groupId = selectedGroupInfo.id;
      const response = await fetch(`/api/groups/${groupId}/members/${userId}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        toast.success('Member removed from group');
        
        // Refresh group members
        const membersResponse = await fetch(`/api/groups/${groupId}/members`, {
          headers: {
            'Authorization': `Bearer ${token}`
          }
        });
        
        if (membersResponse.ok) {
          const membersData = await membersResponse.json();
          setGroupMembers(membersData.data || []);
        }
      } else {
        toast.error('Failed to remove member');
      }
    } catch (error) {
      console.error('Failed to remove member:', error);
      toast.error('Failed to remove member');
    }
  };

  const handleDeleteGroup = async () => {
    if (!selectedGroupInfo) return;
    if (!window.confirm('Are you sure you want to delete this group? This cannot be undone.')) return;
    try {
      const response = await fetch(`/api/groups/${selectedGroupInfo.id}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      if (response.ok) {
        toast.success('Group deleted');
        setGroupInfoModalOpen(false);
        await fetchGroups();
      } else {
        toast.error('Failed to delete group');
      }
    } catch (error) {
      toast.error('Failed to delete group');
    }
  };

  const handleLeaveGroup = async () => {
    if (!selectedGroupInfo) return;
    if (!window.confirm('Are you sure you want to leave this group?')) return;
    
    try {
      const response = await fetch(`/api/groups/${selectedGroupInfo.id}/leave`, {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        toast.success('Left group successfully');
        setGroupInfoModalOpen(false);
        await fetchGroups(); // Refresh groups list
      } else {
        const error = await response.json();
        toast.error(error.message || 'Failed to leave group');
      }
    } catch (error) {
      console.error('Failed to leave group:', error);
      toast.error('Failed to leave group');
    }
  };

  const getFilteredChats = () => {
    const currentChats = activeTab === 'chats' ? chats : groups;
    return currentChats.filter(chat =>
      chat.name.toLowerCase().includes(searchQuery.toLowerCase())
    );
  };

  const filteredChats = getFilteredChats();

  // Expose refreshGroups method to parent
  useImperativeHandle(ref, () => ({
    refreshGroups: fetchGroups
  }));

  return (
    <div className="flex-1 flex flex-col">
      {/* Search Bar */}
      <div className="p-4 border-b border-dark-700">
        <div className="relative">
          <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 w-4 h-4 text-dark-400" />
          <input
            type="text"
            placeholder="Search conversations..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="input-field w-full pl-10"
          />
        </div>
      </div>

      {/* Tabs */}
      <div className="flex border-b border-dark-700">
        <button
          onClick={() => setActiveTab('chats')}
          className={`flex-1 py-3 px-4 text-sm font-medium transition-colors ${
            activeTab === 'chats'
              ? 'text-white border-b-2 border-cockpit-400'
              : 'text-dark-400 hover:text-white'
          }`}
        >
          <MessageCircle className="w-4 h-4 inline mr-2" />
          Chats
        </button>
        <button
          onClick={() => setActiveTab('groups')}
          className={`flex-1 py-3 px-4 text-sm font-medium transition-colors ${
            activeTab === 'groups'
              ? 'text-white border-b-2 border-cockpit-400'
              : 'text-dark-400 hover:text-white'
          }`}
        >
          <Users className="w-4 h-4 inline mr-2" />
          Groups
        </button>
      </div>

      {/* Chat List */}
      <div className="flex-1 overflow-y-auto">
        {isLoading ? (
          <div className="p-8 text-center">
            <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-cockpit-400 mx-auto"></div>
            <p className="text-dark-400 mt-2">Loading...</p>
          </div>
        ) : filteredChats.length === 0 ? (
          <div className="p-8 text-center">
            <MessageCircle className="w-12 h-12 text-dark-600 mx-auto mb-4" />
            <p className="text-dark-400">
              {activeTab === 'chats' ? 'No conversations yet' : 'No groups yet'}
            </p>
            {activeTab === 'groups' && (
              <button
                onClick={handleCreateGroup}
                className="btn-primary mt-4 text-sm"
              >
                Create Group
              </button>
            )}
          </div>
        ) : (
          <div className="p-2">
            {filteredChats.map((chat) => (
              <div
                key={chat.id}
                onClick={() => chat.isGroup ? handleGroupClick(chat) : onChatSelect(chat.id)}
                className={`p-3 rounded-lg cursor-pointer transition-colors ${
                  selectedChat === chat.id
                    ? 'bg-cockpit-600/20 border border-cockpit-600/30'
                    : 'hover:bg-dark-800'
                }`}
              >
                <div className="flex items-center space-x-3">
                  <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
                    {chat.isGroup ? (
                      <Users className="w-5 h-5 text-white" />
                    ) : (
                      <span className="text-white font-medium text-sm">
                        {chat.name.charAt(0).toUpperCase()}
                      </span>
                    )}
                  </div>
                  <div className="flex-1 min-w-0">
                    <h3 className="text-sm font-medium text-white truncate">
                      {chat.name}
                    </h3>
                    <p className="text-xs text-dark-400 truncate">
                      {chat.lastMessage}
                    </p>
                  </div>
                  <div className="text-right">
                    <p className="text-xs text-dark-400">
                      {chat.timestamp}
                    </p>
                    {chat.unreadCount > 0 && (
                      <div className="mt-1 w-5 h-5 bg-cockpit-400 rounded-full flex items-center justify-center">
                        <span className="text-xs text-white font-medium">
                          {chat.unreadCount}
                        </span>
                      </div>
                    )}
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>

      {/* Bottom Action Button */}
      <div className="p-4 border-t border-dark-700">
        {activeTab === 'groups' ? (
          <button 
            onClick={handleCreateGroup}
            className="btn-primary w-full flex items-center justify-center space-x-2"
          >
            <Plus className="w-4 h-4" />
            <span>Create Group</span>
          </button>
        ) : (
          <button
            onClick={handleNewConversation}
            className="btn-primary w-full flex items-center justify-center space-x-2"
          >
            <Plus className="w-4 h-4" />
            <span>New Conversation</span>
          </button>
        )}
      </div>

      {/* Group Creation Modal */}
      <Modal isOpen={isGroupModalOpen} onClose={() => setGroupModalOpen(false)} title="Create Group">
        <form onSubmit={submitCreateGroup} className="space-y-4">
          <div>
            <label className="block text-sm text-dark-300 mb-1">Group Name</label>
            <input
              type="text"
              value={groupName}
              onChange={e => setGroupName(e.target.value)}
              className="input-field w-full"
              required
              maxLength={50}
              placeholder="Enter group name"
            />
          </div>
          <div>
            <label className="block text-sm text-dark-300 mb-1">Description</label>
            <textarea
              value={groupDescription}
              onChange={e => setGroupDescription(e.target.value)}
              className="input-field w-full"
              maxLength={200}
              placeholder="Enter group description (optional)"
            />
          </div>
          <div className="flex justify-end space-x-2">
            <button type="button" className="btn-secondary" onClick={() => setGroupModalOpen(false)}>
              Cancel
            </button>
            <button type="submit" className="btn-primary" disabled={isCreatingGroup}>
              {isCreatingGroup ? 'Creating...' : 'Create'}
            </button>
          </div>
        </form>
      </Modal>

      {/* Group Info Modal */}
      <Modal isOpen={isGroupInfoModalOpen} onClose={() => setGroupInfoModalOpen(false)} title="Group Info">
        {selectedGroupInfo && (
          <div className="space-y-6">
            {/* Group Info */}
            <div className="space-y-4">
              <div>
                <h3 className="text-lg font-semibold text-white">{selectedGroupInfo.name}</h3>
                <p className="text-dark-400 text-sm">{selectedGroupInfo.description || 'No description'}</p>
              </div>
              
              <div className="flex items-center justify-between text-sm">
                <span className="text-dark-400">Created:</span>
                <span className="text-white">{new Date(selectedGroupInfo.created_at).toLocaleDateString()}</span>
              </div>
              
              <div className="flex items-center justify-between text-sm">
                <span className="text-dark-400">Members:</span>
                <span className="text-white">{groupMembers.length}</span>
              </div>
              <div className="flex space-x-2">
                <button
                  onClick={handleLeaveGroup}
                  className="btn-secondary w-full flex items-center justify-center space-x-2"
                >
                  <LogOut className="w-4 h-4" />
                  <span>Leave Group</span>
                </button>
                {selectedGroupInfo.is_admin && (
                  <button
                    onClick={handleDeleteGroup}
                    className="btn-danger w-full flex items-center justify-center space-x-2"
                  >
                    <Trash2 className="w-4 h-4" />
                    <span>Delete Group</span>
                  </button>
                )}
              </div>
            </div>

            {/* Invite User */}
            <div className="border-t border-dark-700 pt-4">
              <h4 className="text-sm font-medium text-white mb-3">Invite User</h4>
              <form onSubmit={handleInviteUser} className="space-y-3">
                <input
                  type="text"
                  value={inviteUsername}
                  onChange={(e) => setInviteUsername(e.target.value)}
                  className="input-field w-full"
                  placeholder="Enter username to invite"
                  required
                />
                <button
                  type="submit"
                  disabled={isInvitingUser}
                  className="btn-primary w-full flex items-center justify-center space-x-2"
                >
                  <UserPlus className="w-4 h-4" />
                  <span>{isInvitingUser ? 'Inviting...' : 'Invite User'}</span>
                </button>
              </form>
            </div>

            {/* Group Members */}
            <div className="border-t border-dark-700 pt-4">
              <h4 className="text-sm font-medium text-white mb-3">Members</h4>
              <div className="space-y-2 max-h-40 overflow-y-auto">
                {groupMembers.map((member: any) => (
                  <div key={member.id} className="flex items-center justify-between p-2 bg-dark-800 rounded">
                    <div className="flex items-center space-x-2">
                      <div className="w-6 h-6 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
                        <span className="text-white text-xs font-medium">
                          {member.username.charAt(0).toUpperCase()}
                        </span>
                      </div>
                      <span className="text-white text-sm">{member.username}</span>
                      {member.role === 'admin' && (
                        <span className="text-xs bg-cockpit-600 text-white px-2 py-1 rounded">Admin</span>
                      )}
                    </div>
                    {selectedGroupInfo.is_admin && member.role !== 'admin' && (
                      <button
                        onClick={() => handleRemoveMember(member.id)}
                        className="p-1 text-red-400 hover:text-red-300 transition-colors"
                        title="Remove member"
                      >
                        <Trash2 className="w-4 h-4" />
                      </button>
                    )}
                  </div>
                ))}
              </div>
            </div>
          </div>
        )}
      </Modal>

      {/* New Conversation Modal */}
      <Modal isOpen={isNewChatModalOpen} onClose={() => setNewChatModalOpen(false)} title="Start New Conversation">
        <div className="space-y-4">
          <div>
            <label className="block text-sm text-dark-300 mb-1">Search Users</label>
            <input
              type="text"
              value={newChatUser}
              onChange={e => {
                setNewChatUser(e.target.value);
                handleUserSearch(e.target.value);
              }}
              className="input-field w-full"
              placeholder="Enter username to search"
            />
          </div>
          
          {/* Search Results */}
          {isSearching && (
            <div className="text-center py-4">
              <div className="text-dark-400 text-sm">Searching...</div>
            </div>
          )}
          
          {!isSearching && searchResults.length > 0 && (
            <div>
              <h4 className="text-sm font-medium text-white mb-2">Found Users ({searchResults.length})</h4>
              <div className="space-y-2 max-h-40 overflow-y-auto">
                {searchResults.map((user: any) => {
                  console.log('Rendering user:', user);
                  return (
                    <div key={user.id} className="flex items-center justify-between p-2 bg-dark-800 rounded">
                      <div className="flex items-center space-x-2">
                        <div className="w-8 h-8 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
                          <span className="text-white text-sm font-medium">
                            {user.username.charAt(0).toUpperCase()}
                          </span>
                        </div>
                        <div>
                          <span className="text-white text-sm font-medium">{user.username}</span>
                          <div className="text-dark-400 text-xs">{user.email}</div>
                        </div>
                      </div>
                      <button
                        onClick={() => handleStartConversation(user.id, user.username)}
                        className="btn-primary px-3 py-1 text-xs"
                      >
                        Start Chat
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>
          )}
          
          {!isSearching && newChatUser.trim() && searchResults.length === 0 && (
            <div className="text-center py-4">
              <div className="text-dark-400 text-sm">No users found</div>
            </div>
          )}
          
          <div className="flex justify-end space-x-2">
            <button type="button" className="btn-secondary" onClick={() => {
              setNewChatModalOpen(false);
              setNewChatUser('');
              setSearchResults([]);
            }}>
              Cancel
            </button>
          </div>
        </div>
      </Modal>
    </div>
  );
});

export default Sidebar; 