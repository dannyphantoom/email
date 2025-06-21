import React, { useState, useEffect, useRef } from 'react';
import { Send, Paperclip, Smile, MoreVertical, Download, Upload, Users, MessageCircle, Trash2, X } from 'lucide-react';
import { format } from 'date-fns';
import { toast } from 'react-hot-toast';
import { useAuthStore } from '../stores/authStore';
import Modal from './Modal';

interface Message {
  id: string;
  content: string;
  sender: string;
  timestamp: Date;
  isOwn: boolean;
  type: 'text' | 'file' | 'image';
  fileName?: string;
  fileSize?: number;
}

interface ChatWindowProps {
  chatId: string;
}

const ChatWindow: React.FC<ChatWindowProps> = ({ chatId }) => {
  const { token } = useAuthStore();
  const [messages, setMessages] = useState<Message[]>([]);
  const [newMessage, setNewMessage] = useState('');
  const [isTyping, setIsTyping] = useState(false);
  const [chatInfo, setChatInfo] = useState<any>(null);
  const [isGroup, setIsGroup] = useState(false);
  const [groupMembers, setGroupMembers] = useState<any[]>([]);
  const [isGroupInfoModalOpen, setGroupInfoModalOpen] = useState(false);
  const [inviteUsername, setInviteUsername] = useState('');
  const [isInvitingUser, setIsInvitingUser] = useState(false);
  const [showEmojiPicker, setShowEmojiPicker] = useState(false);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [isUploading, setIsUploading] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);

  // Parse chat ID to determine if it's a group or direct chat
  useEffect(() => {
    if (chatId.startsWith('group-')) {
      setIsGroup(true);
      const groupId = chatId.replace('group-', '');
      fetchGroupInfo(groupId);
      fetchGroupMessages(groupId);
    } else if (chatId.startsWith('chat-')) {
      setIsGroup(false);
      const sessionId = chatId.replace('chat-', '');
      fetchDirectMessages(sessionId);
    }
  }, [chatId]);

  const fetchGroupInfo = async (groupId: string) => {
    try {
      const response = await fetch(`/api/groups/${groupId}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        setChatInfo(data.data);
      }
    } catch (error) {
      console.error('Failed to fetch group info:', error);
    }
  };

  const fetchGroupMessages = async (groupId: string) => {
    try {
      const response = await fetch(`/api/groups/${groupId}/messages`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const formattedMessages = data.data.map((msg: any) => ({
          id: msg.id.toString(),
          content: msg.content,
          sender: msg.sender_name || 'Unknown',
          timestamp: new Date(msg.timestamp),
          isOwn: msg.sender_id === getCurrentUserId(),
          type: msg.message_type || 'text'
        }));
        setMessages(formattedMessages);
      }
    } catch (error) {
      console.error('Failed to fetch group messages:', error);
      toast.error('Failed to load messages');
    }
  };

  const fetchDirectMessages = async (sessionId: string) => {
    try {
      const response = await fetch(`/chat-sessions/${sessionId}/messages`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const formattedMessages = data.data.map((msg: any) => ({
          id: msg.id.toString(),
          content: msg.content,
          sender: msg.sender_name || 'Unknown',
          timestamp: new Date(msg.timestamp),
          isOwn: msg.sender_id === getCurrentUserId(),
          type: msg.message_type || 'text'
        }));
        setMessages(formattedMessages);
      }
    } catch (error) {
      console.error('Failed to fetch direct messages:', error);
      toast.error('Failed to load messages');
    }
  };

  const getCurrentUserId = () => {
    // Use the auth store to get the current user ID
    const { user } = useAuthStore.getState();
    return user ? user.id.toString() : null;
  };

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSendMessage = async () => {
    if (!newMessage.trim()) return;

    const messageData = {
      content: newMessage,
      type: 'text'
    };

    try {
      let response;
      if (isGroup) {
        const groupId = chatId.replace('group-', '');
        response = await fetch(`/api/groups/${groupId}/messages`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`
          },
          body: JSON.stringify(messageData)
        });
      } else {
        const sessionId = chatId.replace('chat-', '');
        response = await fetch(`/chat-sessions/${sessionId}/messages`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`
          },
          body: JSON.stringify(messageData)
        });
      }

      if (response.ok) {
        // Clear the input first
        setNewMessage('');
        
        // Refresh messages from server to get the actual saved message
        if (isGroup) {
          const groupId = chatId.replace('group-', '');
          await fetchGroupMessages(groupId);
        } else {
          const sessionId = chatId.replace('chat-', '');
          await fetchDirectMessages(sessionId);
        }
      } else {
        const errorData = await response.json().catch(() => ({}));
        console.error('Failed to send message:', response.status, errorData);
        toast.error('Failed to send message');
      }
    } catch (error) {
      console.error('Failed to send message:', error);
      toast.error('Failed to send message');
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSendMessage();
    }
  };

  const handleBackupChat = async () => {
    const backupName = prompt('Enter backup name:');
    if (!backupName) return;

    const backupData = {
      chatId,
      messages,
      chatInfo,
      timestamp: new Date().toISOString()
    };

    try {
      const response = await fetch('/api/backup', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
          name: backupName,
          data: JSON.stringify(backupData),
          description: `Backup of ${isGroup ? 'group' : 'chat'} conversation`
        })
      });

      if (response.ok) {
        toast.success('Chat backup created successfully');
      } else {
        toast.error('Failed to create backup');
      }
    } catch (error) {
      console.error('Failed to create backup:', error);
      toast.error('Failed to create backup');
    }
  };

  const handleRestoreChat = async () => {
    try {
      const response = await fetch('/api/backup', {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      if (response.ok) {
        const data = await response.json();
        const backups = data.data;
        
        if (backups.length === 0) {
          toast.error('No backups available');
          return;
        }

        // Show backup selection dialog
        const backupNames = backups.map((b: any) => b.name);
        const selectedBackup = prompt('Select backup to restore:\n' + backupNames.join('\n'));
        
        if (!selectedBackup) return;

        const backup = backups.find((b: any) => b.name === selectedBackup);
        if (!backup) {
          toast.error('Backup not found');
          return;
        }

        const restoreResponse = await fetch(`/api/backup/${backup.id}/restore`, {
          method: 'POST',
          headers: {
            'Authorization': `Bearer ${token}`
          }
        });

        if (restoreResponse.ok) {
          toast.success('Chat restored successfully');
          // Reload messages
          if (isGroup) {
            const groupId = chatId.replace('group-', '');
            fetchGroupMessages(groupId);
          } else {
            const sessionId = chatId.replace('chat-', '');
            fetchDirectMessages(sessionId);
          }
        } else {
          toast.error('Failed to restore chat');
        }
      }
    } catch (error) {
      console.error('Failed to restore chat:', error);
      toast.error('Failed to restore chat');
    }
  };

  const handleGroupHeaderClick = async () => {
    if (!isGroup) return;
    
    try {
      const groupId = chatId.replace('group-', '');
      const response = await fetch(`/api/groups/${groupId}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      if (response.ok) {
        const data = await response.json();
        setChatInfo(data.data);
        // Fetch group members
        const membersResponse = await fetch(`/api/groups/${groupId}/members`, {
          headers: {
            'Authorization': `Bearer ${token}`
          }
        });
        if (membersResponse.ok) {
          const membersData = await membersResponse.json();
          setGroupMembers(membersData.data || []);
        }
        setGroupInfoModalOpen(true);
      }
    } catch (error) {
      console.error('Failed to fetch group info:', error);
    }
  };

  const handleInviteUser = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!inviteUsername.trim()) return;
    setIsInvitingUser(true);
    try {
      const groupId = chatId.replace('group-', '');
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
      const groupId = chatId.replace('group-', '');
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
    if (!chatInfo) return;
    if (!window.confirm('Are you sure you want to delete this group? This cannot be undone.')) return;
    try {
      const groupId = chatId.replace('group-', '');
      const response = await fetch(`/api/groups/${groupId}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      if (response.ok) {
        toast.success('Group deleted');
        setGroupInfoModalOpen(false);
        // Redirect to main page or refresh
        window.location.reload();
      } else {
        toast.error('Failed to delete group');
      }
    } catch (error) {
      toast.error('Failed to delete group');
    }
  };

  // Emoji picker component
  const EmojiPicker = () => {
    const emojis = ['😀', '😃', '😄', '😁', '😆', '😅', '😂', '🤣', '😊', '😇', '🙂', '🙃', '😉', '😌', '😍', '🥰', '😘', '😗', '😙', '😚', '😋', '😛', '😝', '😜', '🤪', '🤨', '🧐', '🤓', '😎', '🤩', '🥳', '😏', '😒', '😞', '😔', '😟', '😕', '🙁', '☹️', '😣', '😖', '😫', '😩', '🥺', '😢', '😭', '😤', '😠', '😡', '🤬', '🤯', '😳', '🥵', '🥶', '😱', '😨', '😰', '😥', '😓', '🤗', '🤔', '🤭', '🤫', '🤥', '😶', '😐', '😑', '😯', '😦', '😧', '😮', '😲', '🥱', '😴', '🤤', '😪', '😵', '🤐', '🥴', '🤢', '🤮', '🤧', '😷', '🤒', '🤕', '🤑', '🤠', '💩', '👻', '💀', '☠️', '👽', '👾', '🤖', '😺', '😸', '😹', '😻', '😼', '😽', '🙀', '😿', '😾', '🙈', '🙉', '🙊', '👶', '👧', '🧒', '👦', '👩', '🧑', '👨', '👵', '🧓', '👴', '👮‍♀️', '👮', '👮‍♂️', '🕵️‍♀️', '🕵️', '🕵️‍♂️', '💂‍♀️', '💂', '💂‍♂️', '👷‍♀️', '👷', '👷‍♂️', '🤴', '👸', '👳‍♀️', '👳', '👳‍♂️', '👲', '🧕', '🤵', '👰', '🤰', '🤱', '👼', '🎅', '🤶', '🧙‍♀️', '🧙', '🧙‍♂️', '🧝‍♀️', '🧝', '🧝‍♂️', '🧛‍♀️', '🧛', '🧛‍♂️', '🧟‍♀️', '🧟', '🧟‍♂️', '🧞‍♀️', '🧞', '🧞‍♂️', '🧜‍♀️', '🧜', '🧜‍♂️', '🧚‍♀️', '🧚', '🧚‍♂️', '👼', '🤰', '🤱', '👼', '🎅', '🤶', '🧙‍♀️', '🧙', '🧙‍♂️', '🧝‍♀️', '🧝', '🧝‍♂️', '🧛‍♀️', '🧛', '🧛‍♂️', '🧟‍♀️', '🧟', '🧟‍♂️', '🧞‍♀️', '🧞', '🧞‍♂️', '🧜‍♀️', '🧜', '🧜‍♂️', '🧚‍♀️', '🧚', '🧚‍♂️'];

    const handleEmojiClick = (emoji: string) => {
      setNewMessage(prev => prev + emoji);
      setShowEmojiPicker(false);
    };

    return (
      <div className="absolute bottom-full right-0 mb-2 bg-dark-800 border border-dark-600 rounded-lg p-2 shadow-lg z-10">
        <div className="grid grid-cols-8 gap-1 max-h-48 overflow-y-auto">
          {emojis.map((emoji, index) => (
            <button
              key={index}
              onClick={() => handleEmojiClick(emoji)}
              className="w-8 h-8 flex items-center justify-center hover:bg-dark-700 rounded text-lg"
            >
              {emoji}
            </button>
          ))}
        </div>
      </div>
    );
  };

  // File handling functions
  const handleFileSelect = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file) {
      // Check file size (max 10MB)
      if (file.size > 10 * 1024 * 1024) {
        toast.error('File size must be less than 10MB');
        return;
      }
      setSelectedFile(file);
    }
  };

  const handleFileUpload = async () => {
    if (!selectedFile) return;

    setIsUploading(true);
    try {
      const formData = new FormData();
      formData.append('file', selectedFile);
      formData.append('type', selectedFile.type.startsWith('image/') ? 'image' : 'file');

      let response;
      if (isGroup) {
        const groupId = chatId.replace('group-', '');
        response = await fetch(`/api/groups/${groupId}/messages`, {
          method: 'POST',
          headers: {
            'Authorization': `Bearer ${token}`
          },
          body: formData
        });
      } else {
        const sessionId = chatId.replace('chat-', '');
        response = await fetch(`/chat-sessions/${sessionId}/messages`, {
          method: 'POST',
          headers: {
            'Authorization': `Bearer ${token}`
          },
          body: formData
        });
      }

      if (response.ok) {
        setSelectedFile(null);
        if (fileInputRef.current) {
          fileInputRef.current.value = '';
        }
        
        // Refresh messages
        if (isGroup) {
          const groupId = chatId.replace('group-', '');
          await fetchGroupMessages(groupId);
        } else {
          const sessionId = chatId.replace('chat-', '');
          await fetchDirectMessages(sessionId);
        }
        
        toast.success('File uploaded successfully');
      } else {
        toast.error('Failed to upload file');
      }
    } catch (error) {
      console.error('Failed to upload file:', error);
      toast.error('Failed to upload file');
    } finally {
      setIsUploading(false);
    }
  };

  const removeSelectedFile = () => {
    setSelectedFile(null);
    if (fileInputRef.current) {
      fileInputRef.current.value = '';
    }
  };

  const formatFileSize = (bytes: number) => {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  return (
    <div className="flex-1 flex flex-col">
      {/* Chat Header */}
      <div className="bg-dark-900 border-b border-dark-700 p-4">
        <div className="flex items-center justify-between">
          <div 
            className={`flex items-center space-x-3 ${isGroup ? 'cursor-pointer hover:bg-dark-800 p-2 rounded-lg transition-colors' : ''}`}
            onClick={isGroup ? handleGroupHeaderClick : undefined}
          >
            <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
              {isGroup ? (
                <Users className="w-5 h-5 text-white" />
              ) : (
                <span className="text-white font-medium text-sm">
                  {chatInfo?.name?.charAt(0).toUpperCase() || 'U'}
                </span>
              )}
            </div>
            <div>
              <h2 className="text-lg font-semibold text-white">
                {chatInfo?.name || 'Loading...'}
              </h2>
              <p className="text-sm text-dark-400">
                {isTyping ? 'Typing...' : (isGroup ? `${groupMembers.length} members` : 'Online')}
              </p>
            </div>
          </div>
          <div className="flex items-center space-x-2">
            <button
              onClick={handleBackupChat}
              className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
              title="Backup Chat"
            >
              <Download className="w-4 h-4" />
            </button>
            <button
              onClick={handleRestoreChat}
              className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
              title="Restore Chat"
            >
              <Upload className="w-4 h-4" />
            </button>
            <button className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors">
              <MoreVertical className="w-5 h-5" />
            </button>
          </div>
        </div>
      </div>

      {/* Messages Area */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4 chat-mesh-bg">
        {messages.length === 0 ? (
          <div className="text-center py-8">
            <MessageCircle className="w-12 h-12 text-dark-600 mx-auto mb-4" />
            <p className="text-dark-400">No messages yet</p>
            <p className="text-sm text-dark-500">Start the conversation!</p>
          </div>
        ) : (
          messages.map((message) => (
            <div
              key={message.id}
              className={`flex ${message.isOwn ? 'justify-end' : 'justify-start'}`}
            >
              <div className={`message-bubble ${
                message.isOwn ? 'message-bubble-sent' : 'message-bubble-received'
              }`}>
                {message.type === 'file' || message.type === 'image' ? (
                  <div className="space-y-2">
                    {message.type === 'image' ? (
                      <img 
                        src={message.content} 
                        alt="Shared image" 
                        className="max-w-xs rounded-lg"
                        onError={(e) => {
                          e.currentTarget.src = 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMjAwIiBoZWlnaHQ9IjIwMCIgdmlld0JveD0iMCAwIDIwMCAyMDAiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+CjxyZWN0IHdpZHRoPSIyMDAiIGhlaWdodD0iMjAwIiBmaWxsPSIjMzc0MTUxIi8+CjxwYXRoIGQ9Ik01MCA1MEgxNTBWMTUwSDUwVjUwWiIgZmlsbD0iIzY3NzM4MCIvPgo8cGF0aCBkPSJNNzAgNzBIMTMwVjEzMEg3MFY3MFoiIGZpbGw9IiM2NzczODAiLz4KPC9zdmc+';
                        }}
                      />
                    ) : (
                      <div className="flex items-center space-x-2 p-3 bg-dark-700 rounded-lg">
                        <Paperclip className="w-5 h-5 text-cockpit-400" />
                        <div className="flex-1 min-w-0">
                          <p className="text-sm font-medium text-white truncate">
                            {message.fileName || 'File'}
                          </p>
                          {message.fileSize && (
                            <p className="text-xs text-dark-400">
                              {formatFileSize(message.fileSize)}
                            </p>
                          )}
                        </div>
                        <a 
                          href={message.content} 
                          download={message.fileName}
                          className="p-1 text-cockpit-400 hover:text-cockpit-300 transition-colors"
                          title="Download file"
                        >
                          <Download className="w-4 h-4" />
                        </a>
                      </div>
                    )}
                    <p className={`text-xs mt-1 ${
                      message.isOwn ? 'text-white/70' : 'text-dark-300'
                    }`}>
                      {format(message.timestamp, 'HH:mm')}
                    </p>
                  </div>
                ) : (
                  <>
                    <p className="text-sm">{message.content}</p>
                    <p className={`text-xs mt-1 ${
                      message.isOwn ? 'text-white/70' : 'text-dark-300'
                    }`}>
                      {format(message.timestamp, 'HH:mm')}
                    </p>
                  </>
                )}
              </div>
            </div>
          ))
        )}
        <div ref={messagesEndRef} />
      </div>

      {/* Message Input */}
      <div className="bg-dark-900 border-t border-dark-700 p-4">
        {/* Selected File Display */}
        {selectedFile && (
          <div className="mb-3 p-3 bg-dark-800 rounded-lg border border-cockpit-600/30">
            <div className="flex items-center justify-between">
              <div className="flex items-center space-x-2">
                <Paperclip className="w-4 h-4 text-cockpit-400" />
                <div className="flex-1 min-w-0">
                  <p className="text-sm font-medium text-white truncate">
                    {selectedFile.name}
                  </p>
                  <p className="text-xs text-dark-400">
                    {formatFileSize(selectedFile.size)}
                  </p>
                </div>
              </div>
              <div className="flex items-center space-x-2">
                <button
                  onClick={handleFileUpload}
                  disabled={isUploading}
                  className="btn-primary px-3 py-1 text-xs"
                >
                  {isUploading ? 'Uploading...' : 'Send'}
                </button>
                <button
                  onClick={removeSelectedFile}
                  className="p-1 text-red-400 hover:text-red-300 transition-colors"
                >
                  <X className="w-4 h-4" />
                </button>
              </div>
            </div>
          </div>
        )}

        <div className="flex items-end space-x-3">
          <div className="flex-1 relative">
            <textarea
              value={newMessage}
              onChange={(e) => setNewMessage(e.target.value)}
              onKeyPress={handleKeyPress}
              placeholder="Type a message..."
              className="input-field w-full resize-none min-h-[44px] max-h-32 py-3 pr-20"
              rows={1}
            />
            <div className="absolute right-2 bottom-2 flex items-center space-x-1">
              <div className="relative">
                <button 
                  onClick={() => setShowEmojiPicker(!showEmojiPicker)}
                  className="p-1 text-dark-400 hover:text-white transition-colors"
                  title="Add emoji"
                >
                  <Smile className="w-4 h-4" />
                </button>
                {showEmojiPicker && <EmojiPicker />}
              </div>
              <button 
                onClick={() => fileInputRef.current?.click()}
                className="p-1 text-dark-400 hover:text-white transition-colors"
                title="Attach file"
              >
                <Paperclip className="w-4 h-4" />
              </button>
              <input
                ref={fileInputRef}
                type="file"
                onChange={handleFileSelect}
                className="hidden"
                accept="image/*,.pdf,.doc,.docx,.txt,.zip,.rar"
              />
            </div>
          </div>
          <button
            onClick={handleSendMessage}
            disabled={!newMessage.trim()}
            className="btn-primary p-3 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <Send className="w-5 h-5" />
          </button>
        </div>
      </div>

      {/* Group Info Modal */}
      <Modal isOpen={isGroupInfoModalOpen} onClose={() => setGroupInfoModalOpen(false)} title="Group Info">
        {chatInfo && (
          <div className="space-y-6">
            {/* Group Info */}
            <div className="space-y-4">
              <div>
                <h3 className="text-lg font-semibold text-white">{chatInfo.name}</h3>
                <p className="text-dark-400 text-sm">{chatInfo.description || 'No description'}</p>
              </div>
              
              <div className="flex items-center justify-between text-sm">
                <span className="text-dark-400">Created:</span>
                <span className="text-white">{new Date(chatInfo.created_at).toLocaleDateString()}</span>
              </div>
              
              <div className="flex items-center justify-between text-sm">
                <span className="text-dark-400">Members:</span>
                <span className="text-white">{groupMembers.length}</span>
              </div>
              {chatInfo.is_admin && (
                <button
                  onClick={handleDeleteGroup}
                  className="btn-danger w-full mt-2 flex items-center justify-center space-x-2"
                >
                  <Trash2 className="w-4 h-4" />
                  <span>Delete Group</span>
                </button>
              )}
            </div>

            {/* Invite User */}
            {chatInfo.is_admin && (
              <div className="border-t border-dark-700 pt-4">
                <h4 className="text-sm font-medium text-white mb-3">Invite User</h4>
                <form onSubmit={handleInviteUser} className="space-y-3">
                  <input
                    type="text"
                    value={inviteUsername}
                    onChange={(e) => setInviteUsername(e.target.value)}
                    placeholder="Enter username"
                    className="input-field w-full"
                    required
                  />
                  <button
                    type="submit"
                    disabled={isInvitingUser}
                    className="btn-primary w-full"
                  >
                    {isInvitingUser ? 'Inviting...' : 'Invite User'}
                  </button>
                </form>
              </div>
            )}

            {/* Members List */}
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
                    {chatInfo.is_admin && member.role !== 'admin' && (
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
    </div>
  );
};

export default ChatWindow; 